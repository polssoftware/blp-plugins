#include <juce_audio_utils/juce_audio_utils.h>
#include "../Plugin.cpp"
#include <iostream>
#include <random>
#include <stdexcept>
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI gui;try{
 require(argc==3,"usage: host plugin.vst3 directory");juce::File dir(argv[2]);juce::VST3PluginFormat fmt;juce::OwnedArray<juce::PluginDescription> types;fmt.findAllTypesForFile(types,argv[1]);require(types.size()==1,"plugin identity");juce::String error;auto plugin=fmt.createInstanceFromDescription(*types[0],48000,257,error);require(plugin!=nullptr,error.toRawUTF8());
 auto reset=[&]{for(auto* p:plugin->getParameters())p->setValueNotifyingHost(p->getDefaultValue());};
 auto param=[&](const juce::String& name){for(auto* p:plugin->getParameters())if(p->getName(100)==name)return p;throw std::runtime_error("missing parameter");};
 for(int rate:{44100,48000}){
  juce::MemoryBlock bytes;require(dir.getChildFile("dry-"+juce::String(rate)+".f32").loadFileAsData(bytes),"input read");int n=int(bytes.getSize()/(2*sizeof(float)));auto* data=(const float*)bytes.getData();std::vector<float> result(2*n);reset();plugin->releaseResources();plugin->setRateAndBufferSizeDetails(rate,257);plugin->prepareToPlay(rate,257);juce::MidiBuffer midi;
  for(int pos=0;pos<n;pos+=257){int count=std::min(257,n-pos);juce::AudioBuffer<float> b(2,count);for(int c=0;c<2;c++)for(int i=0;i<count;i++)b.setSample(c,i,data[2*(pos+i)+c]);plugin->processBlock(b,midi);for(int c=0;c<2;c++)for(int i=0;i<count;i++){float y=b.getSample(c,i);require(std::isfinite(y),"nonfinite render");result[2*(pos+i)+c]=y;}}
  require(dir.getChildFile("updated-"+juce::String(rate)+".f32").replaceWithData(result.data(),result.size()*4),"save render");
  for(const char* path:{"Bypass","Mix"}){reset();param(path)->setValueNotifyingHost(juce::String(path)=="Bypass"?1:0);plugin->prepareToPlay(rate,257);juce::AudioBuffer<float> b(2,257);for(int warm=0;warm<40;warm++){b.clear();plugin->processBlock(b,midi);}for(int i=0;i<257;i++){b.setSample(0,i,.1f);b.setSample(1,i,-.15f);}plugin->processBlock(b,midi);for(int i=0;i<257;i++)require(b.getSample(0,i)==.1f&&b.getSample(1,i)==-.15f,"dry/bypass transparency");}
  Processor direct;juce::AudioBuffer<float>b(2,257);std::mt19937 rng(12);std::uniform_real_distribution<float> noise(-.2,.2);double peak=0,step=0;float prev=0;
  for(int s:{0,35,70,100})for(int g:{0,60,100})for(int m:{0,1}){auto set=[&](const char* id,float value){auto* p=direct.state.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(value));};set("squeeze",float(s));set("gain",float(g));set("mode",float(m));direct.prepareToPlay(rate,257);for(int k=0;k<80;k++){for(int i=0;i<257;i++){float v=noise(rng);b.setSample(0,i,v);b.setSample(1,i,v);}direct.processBlock(b,midi);for(int i=0;i<257;i++){float y=b.getSample(0,i);require(std::isfinite(y)&&fabs(y)<=1.000001,"finite bounded output");require(y==b.getSample(1,i),"stereo symmetry");peak=std::max(peak,double(fabs(y)));}}}
  direct.prepareToPlay(rate,257);for(int k=0;k<300;k++){if(k%4==0)for(auto* p:direct.getParameters())p->setValueNotifyingHost(float(rng()%1001)/1000);for(int i=0;i<257;i++){b.setSample(0,i,.01f);b.setSample(1,i,.01f);}direct.processBlock(b,midi);for(int i=0;i<257;i++){float y=b.getSample(0,i);require(std::isfinite(y),"automation finite");step=std::max(step,double(fabs(y-prev)));prev=y;}}
  std::cout<<"PASS "<<rate<<" Hz: loaded VST3 default render, exact dry/bypass, stereo, parameter extremes, automation. Peak="<<peak<<" max DC automation step="<<step<<std::endl;
 }
 Processor p;juce::MemoryBlock state;p.state.getParameter("squeeze")->setValueNotifyingHost(.37f);p.getStateInformation(state);Processor q;q.setStateInformation(state.getData(),int(state.getSize()));require(abs(q.v("squeeze")-37)<.001,"state roundtrip");std::unique_ptr<juce::AudioProcessorEditor> editor(q.createEditor());require(editor->getWidth()==300&&editor->getHeight()==270,"GUI dimensions");std::cout<<"PASS state recall and unchanged GUI dimensions"<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}

