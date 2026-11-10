#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../Plugin.cpp"
#include <complex>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

void require(bool condition,const char* message) {
    if(!condition) throw std::runtime_error(message);
}
double rootRadius(double a1,double a2) {
    const auto d=std::sqrt(std::complex<double>(a1*a1-4.0*a2));
    return std::max(std::abs((-a1+d)/2.0),std::abs((-a1-d)/2.0));
}
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    try {
        require(argc==3,"usage: EQ3MHostTests plugin.vst3 render-directory");
        const juce::File folder(argv[2]);
        require(folder.createDirectory().wasOk(),"render directory");
        juce::VST3PluginFormat format;
        juce::OwnedArray<juce::PluginDescription> types;
        format.findAllTypesForFile(types,argv[1]);
        require(types.size()==1 && types[0]->name=="BLP EQ3-M","VST3 identity");
        juce::String error;
        auto plugin=format.createInstanceFromDescription(*types[0],44100,257,error);
        require(plugin!=nullptr,error.toRawUTF8());
        auto find=[&](const juce::String& name) {
            for(auto* parameter:plugin->getParameters())
                if(parameter->getName(100)==name) return parameter;
            throw std::runtime_error("Missing parameter "+name.toStdString());
        };
        auto reset=[&] {
            for(auto* parameter:plugin->getParameters())
                parameter->setValueNotifyingHost(parameter->getDefaultValue());
        };
        const char* gainNames[]={"lowGain","midGain","highGain"};
        const double expectedDefaults[]={
            std::log(220.0/40)/std::log(500.0/40),
            std::log(5000.0/100)/std::log(8000.0/100),
            (1.2-.3)/(4-.3),
            std::log(7000.0/1000)/std::log(16000.0/1000)};
        const char* settingNames[]={"lowFreq","midFreq","midQ","highFreq"};
        for(int i=0;i<4;++i)
            require(std::abs(find(settingNames[i])->getDefaultValue()-expectedDefaults[i])<1e-6,"calibrated defaults");

        // Export the same normalized RBJ coefficients used by the DSP. Independent
        // Python RBJ S=1 calculations and actual VST3 sweeps are compared separately.
        juce::FileOutputStream coefficientFile(folder.getChildFile("coefficients.csv"));
        coefficientFile<<"rate,band,gain,b0,b1,b2,a0,a1,a2\n";
        double maximumPole=0,maximumZero=0,maximumDcStep=0,maximumAutomationOutput=0;
        for(double rate:{44100.,48000.}) {
            using C=juce::dsp::IIR::ArrayCoefficients<double>;
            for(int band=0;band<3;++band) {
                std::array<double,6> previous{};
                for(int step=0;step<=600;++step) {
                    const double gainDb=-30.0+step*.1;
                    const double linear=juce::Decibels::decibelsToGain(gainDb);
                    auto c=band==0 ? C::makeLowShelf(rate,220.0,std::sqrt(.5),linear)
                          : band==1 ? C::makePeakFilter(rate,5000.0,1.2,linear)
                                    : C::makeHighShelf(rate,7000.0,std::sqrt(.5),linear);
                    const auto a0=c[3];
                    for(auto& value:c) value/=a0;
                    maximumPole=std::max(maximumPole,rootRadius(c[4],c[5]));
                    maximumZero=std::max(maximumZero,rootRadius(c[1]/c[0],c[2]/c[0]));
                    require(maximumPole<1 && maximumZero<1,"pole/zero stability");
                    if(step>0)
                        for(int offset=0;offset<=8;++offset) {
                            const double amount=offset/8.0;
                            std::array<double,6> between;
                            for(size_t i=0;i<c.size();++i) between[i]=previous[i]+amount*(c[i]-previous[i]);
                            require(rootRadius(between[4],between[5])<1,"coefficient-ramp stability");
                            require(rootRadius(between[1]/between[0],between[2]/between[0])<1,"coefficient-ramp minimum phase");
                        }
                    previous=c;
                    if(step==0 || step==300 || step==600) {
                        coefficientFile<<juce::String(rate,0)<<","<<juce::String(band)<<","<<juce::String(gainDb,1);
                        for(auto value:c) coefficientFile<<","<<juce::String(value,17);
                        coefficientFile<<"\n";
                    }
                }
            }

            const int sweepLength=int(rate*12),count=sweepLength+int(rate*2);
            std::vector<float> input(static_cast<size_t>(count));
            const double ratio=std::log(rate*.49/10.0);
            for(int i=0;i<sweepLength;++i) {
                const double phase=juce::MathConstants<double>::twoPi*10*12/ratio
                    *std::expm1((i/rate)*ratio/12);
                const double fade=std::min({1.0,i/(rate*.02),(sweepLength-1-i)/(rate*.02)});
                input[static_cast<size_t>(i)]=float(.0001*fade*std::sin(phase));
            }
            auto save=[&](const juce::String& name,const std::vector<float>& data) {
                require(folder.getChildFile(name+"-"+juce::String(int(rate))+".f32")
                    .replaceWithData(data.data(),data.size()*sizeof(float)),"render save");
            };
            save("input",input);
            for(int band=0;band<3;++band) {
                plugin->releaseResources();
                reset();
                find(gainNames[band])->setValueNotifyingHost(1.0f);
                plugin->setRateAndBufferSizeDetails(rate,257);
                plugin->prepareToPlay(rate,257);
                juce::AudioBuffer<float> audio(2,257);
                juce::MidiBuffer midi;
                for(int block=0;block<int(rate/257)+1;++block) {
                    audio.clear(); plugin->processBlock(audio,midi);
                }
                std::vector<float> output(static_cast<size_t>(count));
                for(int offset=0;offset<count;) {
                    const int length=juce::jmin(257,count-offset);
                    audio.setSize(2,length,false,false,true);
                    for(int channel=0;channel<2;++channel) audio.copyFrom(channel,0,input.data()+offset,length);
                    plugin->processBlock(audio,midi);
                    for(int i=0;i<length;++i) {
                        require(audio.getSample(0,i)==audio.getSample(1,i),"stereo sweep consistency");
                        require(std::isfinite(audio.getSample(0,i)),"nonfinite sweep");
                        output[static_cast<size_t>(offset+i)]=audio.getSample(0,i);
                    }
                    offset+=length;
                }
                save(juce::String("band-")+juce::String(band),output);
            }
            plugin->releaseResources();

            EQ neutral;
            neutral.setRateAndBufferSizeDetails(rate,257);
            neutral.prepareToPlay(rate,257);
            juce::AudioBuffer<float> audio(2,257);
            juce::MidiBuffer midi;
            std::mt19937 random(102);
            std::uniform_real_distribution<float> noise(-1,1);
            for(int block=0;block<20;++block) {
                std::array<float,257> original;
                for(int i=0;i<257;++i) original[size_t(i)]=.01f*noise(random);
                for(int channel=0;channel<2;++channel) audio.copyFrom(channel,0,original.data(),257);
                neutral.processBlock(audio,midi);
                for(int i=0;i<257;++i)
                    require(audio.getSample(0,i)==original[size_t(i)] && audio.getSample(1,i)==original[size_t(i)],
                            "exact neutral unity");
            }
            for(int block=0;block<1200;++block) {
                for(const char* name:{"lowGain","midGain","highGain","midFreq","lowFreq","highFreq","midQ","output"})
                    neutral.state.getParameter(name)->setValueNotifyingHost((noise(random)+1)*.5f);
                neutral.state.getParameter("bypass")->setValueNotifyingHost(block%7==0?1.f:0.f);
                for(int i=0;i<257;++i) {
                    const float value=.000001f*noise(random);
                    audio.setSample(0,i,value);audio.setSample(1,i,value);
                }
                neutral.processBlock(audio,midi);
                for(int i=0;i<257;++i) {
                    const float value=audio.getSample(0,i);
                    require(std::isfinite(value)&&std::abs(value)<1,"automation runaway");
                    require(value==audio.getSample(1,i),"automation stereo mismatch");
                    maximumAutomationOutput=std::max(maximumAutomationOutput,double(std::abs(value)));
                }
            }

            EQ transition;
            transition.prepareToPlay(rate,1);
            juce::AudioBuffer<float> dc(1,1);
            float previous=.0001f;
            for(int i=0;i<int(rate);++i) {
                if(i==int(rate*.1))
                    for(const char* name:gainNames) transition.state.getParameter(name)->setValueNotifyingHost(1.0f);
                if(i==int(rate*.6)) transition.state.getParameter("bypass")->setValueNotifyingHost(1.0f);
                dc.setSample(0,0,.0001f);
                transition.processBlock(dc,midi);
                maximumDcStep=std::max(maximumDcStep,std::abs(double(dc.getSample(0,0)-previous)));
                previous=dc.getSample(0,0);
            }
            require(maximumDcStep<.0002,"parameter transition discontinuity");
            for(int block=0;block<50;++block) {
                for(int i=0;i<257;++i) audio.setSample(0,i,.01f*noise(random));
                const auto original=std::vector<float>(audio.getReadPointer(0),audio.getReadPointer(0)+257);
                transition.processBlockBypassed(audio,midi);
                if(block>20)
                    for(int i=0;i<257;++i)
                        require(std::abs(audio.getSample(0,i)-original[size_t(i)])<1e-7f,"settled bypass");
            }
            std::cout<<"PASS "<<rate<<" Hz: loaded VST3 sweeps, neutral, stereo, coefficient stability, automation, bypass, transitions\n";
        }
        // Stored frequency/Q choices remain intact rather than being silently replaced.
        find("midFreq")->setValueNotifyingHost(1.0f);
        find("midQ")->setValueNotifyingHost(.37f);
        juce::AudioBuffer<float> flush(2,257);flush.clear();juce::MidiBuffer midi;
        plugin->processBlock(flush,midi);
        juce::MemoryBlock state;plugin->getStateInformation(state);
        find("midFreq")->setValueNotifyingHost(0.f);
        plugin->processBlock(flush,midi);
        plugin->setStateInformation(state.getData(),int(state.getSize()));
        require(std::abs(find("midFreq")->getValue()-1.f)<1e-6,"stored mid frequency restore");
        require(std::abs(find("midQ")->getValue()-.37f)<1e-6,"stored Q restore");
        std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditorIfNeeded());
        require(editor && editor->getWidth()==300 && editor->getHeight()==270,"editor layout");
        editor.reset();
        std::cout.precision(12);
        std::cout<<"max_pole_radius="<<maximumPole<<"\nmax_zero_radius="<<maximumZero
                 <<"\nmax_dc_transition_step="<<maximumDcStep
                 <<"\nmax_automation_output="<<maximumAutomationOutput
                 <<"\nPASS saved parameters and editor dimensions\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"FAIL "<<error.what()<<"\n";
        return 1;
    }
}
