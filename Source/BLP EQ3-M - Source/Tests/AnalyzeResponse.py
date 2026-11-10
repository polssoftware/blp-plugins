from pathlib import Path
import csv,json,struct,sys
import numpy as np
renders=Path(sys.argv[1] if len(sys.argv)>1 else Path(__file__).resolve().parent/'renders').resolve()
results=renders.parent/'results'
results.mkdir(exist_ok=True)
names=['Low','Mid','High']
def rbj(rate,band,gain=30.0):
    frequency=[220.,5000.,7000.][band]
    amplitude=10**(gain/40)
    omega=2*np.pi*frequency/rate
    cosine=np.cos(omega)
    sine=np.sin(omega)
    if band==1:
        alpha=sine/(2*1.2)
        numerator=[1+alpha*amplitude,-2*cosine,1-alpha*amplitude]
        denominator=[1+alpha/amplitude,-2*cosine,1-alpha/amplitude]
    else:
        slope=1.0
        alpha=sine/2*np.sqrt((amplitude+1/amplitude)*(1/slope-1)+2)
        beta=2*np.sqrt(amplitude)*alpha
        plus=amplitude+1
        minus=amplitude-1
        if band==0:
            numerator=[amplitude*(plus-minus*cosine+beta),2*amplitude*(minus-plus*cosine),
                       amplitude*(plus-minus*cosine-beta)]
            denominator=[plus+minus*cosine+beta,-2*(minus+plus*cosine),plus+minus*cosine-beta]
        else:
            numerator=[amplitude*(plus+minus*cosine+beta),-2*amplitude*(minus+plus*cosine),
                       amplitude*(plus+minus*cosine-beta)]
            denominator=[plus-minus*cosine+beta,2*(minus-plus*cosine),plus-minus*cosine-beta]
    return np.array(numerator+denominator)/denominator[0]
def transfer(c,rate,frequencies):
    z=np.exp(-2j*np.pi*np.asarray(frequencies)/rate)
    return (c[0]+c[1]*z+c[2]*z*z)/(c[3]+c[4]*z+c[5]*z*z)
def db(value):return 20*np.log10(np.maximum(abs(value),1e-30))
def wav(path,rate,mono):
    data=np.repeat(mono[:,None],2,axis=1).astype('<f4').tobytes()
    fmt=struct.pack('<HHIIHH',3,2,rate,rate*8,8,32)
    chunks=b'fmt '+struct.pack('<I',16)+fmt+b'fact'+struct.pack('<II',4,len(mono))+b'data'+struct.pack('<I',len(data))+data
    path.write_bytes(b'RIFF'+struct.pack('<I',len(chunks)+4)+b'WAVE'+chunks)
coeff_rows=list(csv.DictReader((renders/'coefficients.csv').open()))
coefficients={(int(float(r['rate'])),int(r['band']),float(r['gain'])):
              np.array([float(r[k]) for k in ('b0','b1','b2','a0','a1','a2')]) for r in coeff_rows}
metrics=[]
for rate in (44100,48000):
    dry=np.fromfile(renders/f'input-{rate}.f32',dtype='<f4').astype(float)
    nfft=1<<(len(dry)-1).bit_length()
    bins=np.fft.rfftfreq(nfft,1/rate)
    input_spectrum=np.fft.rfft(dry,nfft)
    mask=(bins>=20)&(bins<=20000)&(abs(input_spectrum)>max(abs(input_spectrum))*1e-5)
    dense=np.geomspace(10,rate*.49999,150000)
    wav(results/f'Input sweep - {rate} Hz.wav',rate,dry)
    for band,name in enumerate(names):
        expected=rbj(rate,band)
        actual=coefficients[rate,band,30.]
        numeric_error=float(max(abs(db(transfer(actual,rate,dense))-db(transfer(expected,rate,dense)))))
        assert numeric_error<1e-7,(rate,name,numeric_error)
        wet=np.fromfile(renders/f'band-{band}-{rate}.f32',dtype='<f4').astype(float)
        assert len(wet)==len(dry) and np.isfinite(wet).all()
        output_spectrum=np.fft.rfft(wet,nfft)
        measured=db(output_spectrum[mask]/input_spectrum[mask])
        target=db(transfer(expected,rate,bins[mask]))
        errors=measured-target
        offset=float(np.mean(errors))
        shape_errors=errors-offset
        metric=dict(rate=rate,band=name,numeric_max_error_db=numeric_error,
                    sweep_max_error_db=float(max(abs(errors))),
                    shape_max_error_db=float(max(abs(shape_errors))),
                    shape_rms_error_db=float(np.sqrt(np.mean(shape_errors**2))),
                    mean_level_offset_db=offset,
                    gain_at_target_frequency_db=float(db(transfer(expected,rate,[220,5000,7000][band]))),
                    rendered_peak_dbfs=float(db(max(abs(wet)))))
        metrics.append(metric)
        assert metric['shape_max_error_db']<.001 and metric['sweep_max_error_db']<.001,metric
        wav(results/f'{name} plus30 - {rate} Hz.wav',rate,wet)
        with (results/f'{name} response - {rate} Hz.csv').open('w',newline='') as file:
            writer=csv.writer(file)
            writer.writerow(['frequency_hz','target_db','vst3_sweep_db','absolute_error_db','shape_error_db'])
            for index in np.searchsorted(bins,np.geomspace(20,20000,401)):
                frequency=bins[index]
                target_value=float(db(transfer(expected,rate,frequency)))
                measured_value=float(db(output_spectrum[index]/input_spectrum[index]))
                writer.writerow([frequency,target_value,measured_value,measured_value-target_value,measured_value-target_value-offset])
with (results/'Endpoint errors.csv').open('w',newline='') as file:
    writer=csv.DictWriter(file,fieldnames=list(metrics[0]))
    writer.writeheader();writer.writerows(metrics)
(results/'metrics.json').write_text(json.dumps(metrics,indent=2))
for row in metrics:print(row,flush=True)
