#include "EqDSP.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>
int main(int argc,char** argv) {
    if(argc!=2) return 1;
    const std::filesystem::path folder(argv[1]);
    std::filesystem::create_directories(folder);
    std::ofstream coefficients(folder/"coefficients.csv");
    coefficients << std::setprecision(17) << "rate,band,db,b0,b1,b2,a1,a2\n";
    for(double rate : {44100.,48000.}) {
        for(size_t band=0;band<7;++band)
            for(double gain : {-15.,-7.5,0.,7.5,15.}) {
                coefficients<<rate<<","<<band<<","<<gain;
                for(auto value:EqDSP::bandCoefficients(rate,band,gain)) coefficients<<","<<value;
                coefficients<<"\n";
            }
        const int length=int(rate*12),tail=int(rate*2),total=length+tail;
        const double start=10,finish=rate*.49,ratio=std::log(finish/start);
        std::vector<float> input(total);
        for(int i=0;i<length;++i) {
            const double time=i/rate;
            const double phase=6.2831853071795864769*start*12/ratio*std::expm1(time*ratio/12);
            // Endpoint fades suppress discontinuity without changing the response estimate.
            const double fade=std::min({1.0,i/(rate*.02),(length-1-i)/(rate*.02)});
            input[i]=float(.0001*fade*std::sin(phase));
        }
        const auto save=[&](const std::string& name,const std::vector<float>& samples) {
            std::ofstream file(folder/name,std::ios::binary);
            file.write(reinterpret_cast<const char*>(samples.data()),samples.size()*sizeof(float));
        };
        save("input-"+std::to_string(int(rate))+".f32",input);
        for(size_t band=0;band<7;++band) {
            auto left=input,right=input;
            EqDSP::Gains gains{};
            gains[band]=15;
            EqDSP dsp;
            dsp.prepare(rate,gains,0,false);
            for(int offset=0;offset<total;) {
                const int samples=std::min(257,total-offset);
                float* data[]={left.data()+offset,right.data()+offset};
                dsp.process(data,2,samples,gains,0,false);
                offset+=samples;
            }
            if(left!=right) return 2;
            save("band-"+std::to_string(band)+"-"+std::to_string(int(rate))+".f32",left);
        }
        for (float scale : {1.0f, 100.0f}) {
            auto signal=input;
            for (auto& value:signal) value*=scale;
            EqDSP::Gains gains;
            gains.fill(15.0f);
            EqDSP dsp;
            dsp.prepare(rate,gains,0,false);
            for (int offset=0;offset<total;) {
                const int samples=std::min(257,total-offset);
                float* data[]={signal.data()+offset};
                dsp.process(data,1,samples,gains,0,false);
                offset+=samples;
            }
            save(std::string(scale==1.0f ? "combined-" : "combined-high-")+std::to_string(int(rate))+".f32",signal);
        }
    }
    return 0;
}
