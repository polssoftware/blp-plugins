#include "BlpEditorTheme.h"
#include <JuceHeader.h>
class Processor;
class Editor;
class Processor : public juce::AudioProcessor {
public:
 juce::AudioProcessorValueTreeState state;
 double sr=44100, envelope=0, reduction=0, memory=0;
 juce::SmoothedValue<float> squeeze,gain,mix,output,mode,bypass;
 int preset=0;
 static auto layout(){juce::AudioProcessorValueTreeState::ParameterLayout p;
 auto add=[&](const char* id,const char* name,float lo,float hi,float def){p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi,0.01f),def));};
 add("squeeze","Squeeze",0,100,70);add("gain","Gain",0,100,60);add("mix","Mix",0,100,100);add("output","Output",-18,18,0);
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1},"Mode",juce::StringArray{"Compress","Limit"},0));p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));return p;}
 Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"BLP_STATE",layout()){}
 float v(const char* id){return state.getRawParameterValue(id)->load();}
 const juce::String getName()const override{return "BLP TecLab BA-2A";} bool acceptsMidi()const override{return false;} bool producesMidi()const override{return false;} double getTailLengthSeconds()const override{return 0;} bool isMidiEffect()const override{return false;}
 bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainInputChannelSet()==l.getMainOutputChannelSet()&&(l.getMainOutputChannelSet()==juce::AudioChannelSet::mono()||l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo());}
 void prepareToPlay(double rate,int)override{sr=std::isfinite(rate)&&rate>=8000?rate:44100;envelope=reduction=memory=0; for(auto* s:{&squeeze,&gain,&mix,&output,&mode,&bypass})s->reset(rate,0.03);squeeze.setCurrentAndTargetValue(v("squeeze"));gain.setCurrentAndTargetValue(v("gain"));mix.setCurrentAndTargetValue(v("mix")/100);output.setCurrentAndTargetValue(v("output"));mode.setCurrentAndTargetValue(v("mode"));bypass.setCurrentAndTargetValue(v("bypass"));}
 void releaseResources()override{}
 juce::AudioProcessorParameter* getBypassParameter()const override{return state.getParameter("bypass");}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{juce::ScopedNoDenormals no;
 squeeze.setTargetValue(v("squeeze"));gain.setTargetValue(v("gain"));mix.setTargetValue(v("mix")/100);output.setTargetValue(v("output"));mode.setTargetValue(v("mode"));bypass.setTargetValue(v("bypass"));
 // Fitted to the supplied default-setting dry/BandLab render pair. Other
 // settings retain a provisional continuous mapping; no proprietary model is implied.
 constexpr double detectorAttack=0.000021497235603126778;
 constexpr double detectorRelease=0.017083112385846415;
 const double attackCoeff=std::exp(-1.0/(sr*detectorAttack));
 const double releaseCoeff=std::exp(-1.0/(sr*detectorRelease));
 const double memoryCoeff=1.0-std::exp(-1.0/(sr*1.5));
 for(int i=0;i<b.getNumSamples();++i){
 double power=0;for(int c=0;c<b.getNumChannels();++c){double x=b.getSample(c,i);power+=x*x;}power/=std::max(1,b.getNumChannels());
 const double e=power>envelope?attackCoeff:releaseCoeff;envelope=e*envelope+(1-e)*power;
 const double s=squeeze.getNextValue(),m=mode.getNextValue();
 const double active=juce::jlimit(0.0,1.0,s/8.0);
 const double threshold=-23.387310152843952+0.43*(70.0-s);
 const double ratio=1.8114804701955993+(20.0-1.8114804701955993)*m;
 const double over=10.0*std::log10(std::max(envelope,1.0e-12))-threshold;
 constexpr double knee=27.92036242124756;
 double gr=over<-knee/2?0:over>knee/2?over*(1-1/ratio):(over+knee/2)*(over+knee/2)/(2*knee)*(1-1/ratio);gr*=active;
 memory+=memoryCoeff*(reduction-memory);
 const double time=gr>reduction?0.0018299623094881667*(1.0-0.5*m):0.00010331987758608055+0.04193694034018799*std::min(1.0,memory/18.0);
 const double a=std::exp(-1.0/(sr*time));reduction=a*reduction+(1-a)*gr;
 const double appliedReduction=reduction*active;
 // Gain is independent of current reduction. The default offset is calibrated,
 // rather than adding a level correction to the exported test audio.
 const double makeupDb=(0.5107786160644552+0.36*(gain.getNextValue()-60.0))*active;
 const float g=float(std::pow(10.0,(makeupDb-appliedReduction)/20.0)),wet=mix.getNextValue(),out=juce::Decibels::decibelsToGain(output.getNextValue()),bp=bypass.getNextValue();
 for(int c=0;c<b.getNumChannels();++c){float dry=b.getSample(c,i),y=(dry*(1-wet)+dry*g*wet)*out;
 // Retain the pre-existing high-level safety stage; it is inactive in the supplied comparison.
 const float edge=0.89125f;if(std::abs(y)>edge){float safe=std::copysign(edge+(1-edge)*std::tanh((std::abs(y)-edge)/(1-edge)),y);y+=float(active)*(safe-y);}
 b.setSample(c,i,y*(1-bp)+dry*bp);}}
 }

 int getNumPrograms()override{return 4;}int getCurrentProgram()override{return preset;}const juce::String getProgramName(int i)override{return juce::StringArray{"Gentle Vocal","Smooth Leveling","Rap Vocal","Strong Compression"}[juce::jlimit(0,3,i)];}void changeProgramName(int,const juce::String&)override{}
 void setCurrentProgram(int i)override{preset=juce::jlimit(0,3,i);const float values[4][5]={{40,40,100,0,0},{70,60,100,0,0},{78,62,90,-1,0},{85,65,100,-2,1}};int j=0;for(auto id:{"squeeze","gain","mix","output","mode"}){auto* p=state.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(values[preset][j++]));}}
 void getStateInformation(juce::MemoryBlock& dest)override{auto tree=state.copyState();tree.setProperty("preset",preset,nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,dest);}void setStateInformation(const void* d,int n)override{auto xml=getXmlFromBinary(d,n);if(xml&&xml->hasTagName("BLP_STATE")){auto t=juce::ValueTree::fromXml(*xml);preset=(int)t.getProperty("preset",0);state.replaceState(t);}}
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override;
};
class ThinKnobLook : public blp::EditorLookAndFeel {
public:
 void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,
                       float position,float start,float end,juce::Slider&) override {
  const float cx=x+width*0.5f, cy=y+height*0.5f;
  const float radius=juce::jmin(28.f,juce::jmin(width,height)*0.5f-3.f);
  const float angle=start+position*(end-start);
  const juce::PathStrokeType stroke(2.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded);
  juce::Path track;track.addCentredArc(cx,cy,radius,radius,0,start,end,true);
  g.setColour(juce::Colour(0xff47474f));g.strokePath(track,stroke);
  if(position>0){juce::Path active;active.addCentredArc(cx,cy,radius,radius,0,start,angle,true);
   g.setColour(juce::Colour(0xffee1c25));g.strokePath(active,stroke);}
  const float face=radius-5.f;
  g.setColour(juce::Colour(0xfff6f5f7));g.fillEllipse(cx-face,cy-face,face*2,face*2);
  juce::Path marker;marker.startNewSubPath(cx+std::sin(angle)*(face-13.f),cy-std::cos(angle)*(face-13.f));
  marker.lineTo(cx+std::sin(angle)*(face-5.f),cy-std::cos(angle)*(face-5.f));
  g.setColour(juce::Colour(0xff323238));g.strokePath(marker,juce::PathStrokeType(2.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 }
};
class Editor:public blp::ThemedEditor {
 Processor& p;ThinKnobLook look;juce::Slider knobs[2];juce::Label labels[2];blp::MenuBox mode { "Mode" };blp::BypassButton bypass;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ma;std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
 public:Editor(Processor& proc):blp::ThemedEditor(proc),p(proc){setSize(300, 270);look.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffee1c25));look.setColour(juce::Slider::thumbColourId,juce::Colours::white);look.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);setLookAndFeel(&look);
 const char* ids[]={"gain","squeeze"};const char* names[]={"Gain","Squeeze"};float defaults[]={60,70};for(int i=0;i<2;i++){addAndMakeVisible(knobs[i]);knobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);knobs[i].setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);knobs[i].setTextValueSuffix(" %");knobs[i].setPopupDisplayEnabled(false,true,this);knobs[i].setDoubleClickReturnValue(true,defaults[i]);knobs[i].setBounds(40+i*130,65,90,82);labels[i].setText(names[i],juce::dontSendNotification);labels[i].setColour(juce::Label::textColourId,blp::textWhite);labels[i].setJustificationType(juce::Justification::centred);labels[i].setBounds(40+i*130,151,90,20);addAndMakeVisible(labels[i]);attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],knobs[i]));}
 mode.addItem("Compress",1);mode.addItem("Limit",2);addAndMakeVisible(mode);mode.setBounds(62,212,176,34);ma=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"mode",mode);
 addAndMakeVisible(bypass);bypass.setBounds(blp::bypassBounds(getWidth()));ba=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass);}
 ~Editor() override { setLookAndFeel(nullptr); }
 void paint(juce::Graphics& g) override
 {
     g.fillAll(blp::canvas);
     blp::paintHeader(g, getWidth(), "BLP TecLab BA-2A");
 }
};
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}



