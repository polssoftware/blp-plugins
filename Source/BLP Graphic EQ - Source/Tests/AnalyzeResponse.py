from pathlib import Path
import csv,json,math,struct,zipfile,sys
import numpy as np
renders=Path(sys.argv[1] if len(sys.argv)>1 else 'response-renders').resolve()
results=renders.parent/'response-results' 
results.mkdir(exist_ok=True)
fits=np.array([
 [95.5658,2.62966,94.0484,.306305],
 [269.5379,2.03929,260.6956,.243495],
 [393.5380,2.88446,387.3492,.337734],
 [788.8211,3.13986,776.4935,.370041],
 [1542.2678,2.43143,1513.3170,.284443],
 [3242.5899,2.88646,3183.5981,.341649]])
labels=[100,200,400,800,1600,3200,6400]
def target_coefficients(rate,band):
    if band==6:
        a=np.longdouble(10)**(np.longdouble('13.29345')/40)
        t=np.tan(np.longdouble(np.pi)*np.longdouble('1740.5777')/rate)
        tz,tp=t/a,t*a
        b=np.array([1+1/tz,1-1/tz,0],dtype=np.longdouble)
        denominator=np.array([1+1/tp,1-1/tp,0],dtype=np.longdouble)
    else:
        fp,qp,fz,qz=[np.longdouble(str(v)) for v in fits[band]]
        def triplet(f,q):
            t=np.tan(np.longdouble(np.pi)*f/rate)
            return np.array([1+1/(t*q)+1/t**2,2-2/t**2,1-1/(t*q)+1/t**2],dtype=np.longdouble)
        b,denominator=triplet(fz,qz),triplet(fp,qp)
    return np.array([*(b/denominator[0]),*(denominator[1:]/denominator[0])],dtype=float)
def response(c,rate,f):
    z=np.exp(-2j*np.pi*np.asarray(f)/rate)
    return (c[0]+c[1]*z+c[2]*z*z)/(1+c[3]*z+c[4]*z*z)
def db(v):return 20*np.log10(np.maximum(abs(v),1e-30))
def write_wav(path,rate,mono):
    data=np.repeat(mono[:,None],2,axis=1).astype('<f4').tobytes()
    fmt=struct.pack('<HHIIHH',3,2,int(rate),int(rate)*8,8,32)
    chunks=b'fmt '+struct.pack('<I',16)+fmt+b'fact'+struct.pack('<II',4,len(mono))+b'data'+struct.pack('<I',len(data))+data
    path.write_bytes(b'RIFF'+struct.pack('<I',len(chunks)+4)+b'WAVE'+chunks)

rows=list(csv.DictReader((renders/'coefficients.csv').open()))
coeff={(int(float(r['rate'])),int(r['band']),float(r['db'])):np.array([float(r[k]) for k in ('b0','b1','b2','a1','a2')]) for r in rows}
metrics=[];curves=[];max_coefficient_error=0
for rate in (44100,48000):
    dry=np.fromfile(renders/f'input-{rate}.f32',dtype='<f4').astype(float)
    write_wav(results/f'Input sweep - {rate} Hz.wav',rate,dry)
    nfft=1<<(len(dry)-1).bit_length()
    x=np.fft.rfft(dry,nfft)
    bins=np.fft.rfftfreq(nfft,1/rate)
    mask=(bins>=20)&(bins<=20000)&(abs(x)>max(abs(x))*1e-5)
    dense=np.geomspace(10,rate*.49999,150000)
    for band in range(7):
        actual=coeff[rate,band,15.]
        expected=target_coefficients(rate,band)
        max_coefficient_error=max(max_coefficient_error,float(max(abs(actual-expected))))
        numeric_error=float(max(abs(db(response(actual,rate,dense))-db(response(expected,rate,dense)))))
        wet=np.fromfile(renders/f'band-{band}-{rate}.f32',dtype='<f4').astype(float)
        write_wav(results/f'{labels[band]} - plus15 - {rate} Hz.wav',rate,wet)
        y=np.fft.rfft(wet,nfft)
        sweep_db=db(y[mask]/x[mask])
        target_db=db(response(expected,rate,bins[mask]))
        errors=sweep_db-target_db
        sweep_error=float(max(abs(errors)))
        rms_error=float(np.sqrt(np.mean(errors**2)))
        peak_db=db(response(expected,rate,dense))
        peak=int(np.argmax(peak_db))
        peak_frequency=float(dense[peak])
        peak_gain=float(peak_db[peak])
        max_output=float(np.max(abs(wet)))
        metric=dict(rate=rate,band=labels[band],numeric_max_error_db=numeric_error,
                    sweep_max_error_db=sweep_error,sweep_rms_error_db=rms_error,
                    peak_frequency_hz=peak_frequency,peak_gain_db=peak_gain,
                    label_gain_db=float(db(response(expected,rate,labels[band]))),
                    rendered_peak_dbfs=float(db(max_output)))
        hosted_path=renders/f'vst3-band-{band}-{rate}.f32'
        if hosted_path.exists():
            hosted=np.fromfile(hosted_path,dtype='<f4').astype(float)
            assert len(hosted)==len(dry)
            host_transform=np.fft.rfft(hosted,nfft)
            host_error=db(host_transform[mask]/x[mask])-target_db
            metric['hosted_sweep_max_error_db']=float(max(abs(host_error)))
            metric['hosted_sweep_rms_error_db']=float(np.sqrt(np.mean(host_error**2)))
            metric['hosted_vs_dsp_max_sample_error']=float(max(abs(hosted-wet)))
            assert metric['hosted_sweep_max_error_db']<.001,(rate,band,metric['hosted_sweep_max_error_db'])
            write_wav(results/f'{labels[band]} - plus15 - {rate} Hz.wav',rate,hosted)
            with (results/f'{labels[band]} - VST3 response - {rate} Hz.csv').open('w',newline='') as file:
                writer=csv.writer(file)
                writer.writerow(['frequency_hz','target_db','vst3_sweep_db','error_db'])
                for index in np.searchsorted(bins,np.geomspace(20,20000,401)):
                    target_value=float(db(response(expected,rate,bins[index])))
                    value=float(db(host_transform[index]/x[index]))
                    writer.writerow([bins[index],target_value,value,value-target_value])
        metrics.append(metric)
        assert numeric_error<.000001,(rate,band,numeric_error)
        assert sweep_error<.001,(rate,band,sweep_error)
        sample_f=np.geomspace(20,20000,401)
        selected=np.searchsorted(bins,sample_f)
        with (results/f'{labels[band]} - response - {rate} Hz.csv').open('w',newline='') as file:
            writer=csv.writer(file)
            writer.writerow(['frequency_hz','target_db','coefficient_db','rendered_sweep_db','sweep_error_db'])
            for index in selected:
                f=bins[index]
                target=float(db(response(expected,rate,f)))
                rendered=float(db(y[index]/x[index]))
                writer.writerow([f,target,float(db(response(actual,rate,f))),rendered,rendered-target])
        curves.append((rate,band,expected))
    combined=sum(db(response(target_coefficients(rate,b),rate,dense)) for b in range(7))
    headroom=float(max(combined))
    combined_low=np.fromfile(renders/f'combined-{rate}.f32',dtype='<f4').astype(float)
    combined_high=np.fromfile(renders/f'combined-high-{rate}.f32',dtype='<f4').astype(float)
    linear_error=float(max(abs(combined_high/100-combined_low)))
    assert linear_error<2e-8,('linearity',rate,linear_error)
    assert max(abs(combined_high))>1,('headroom check did not exceed full scale',rate)
    write_wav(results/f'All bands plus15 - {rate} Hz.wav',rate,combined_low)
    print('COMBINED',rate,'provisional cascade peak',headroom,'dB, high-level peak',
          float(max(abs(combined_high))),'float full scale, normalized linearity error',linear_error,flush=True)

with (results/'Endpoint errors.csv').open('w',newline='') as file:
    writer=csv.DictWriter(file,fieldnames=list(metrics[0]))
    writer.writeheader();writer.writerows(metrics)
summary={'max_coefficient_error':max_coefficient_error,'endpoint_metrics':metrics}
(results/'metrics.json').write_text(json.dumps(summary,indent=2))
print(json.dumps(summary,indent=2),flush=True)
