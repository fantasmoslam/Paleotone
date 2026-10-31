/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;
import java.io.*;
final class WavSample {
    static final long MAX_BYTES=32L*1024*1024;
    final float[] data;final int rate;
    WavSample(float[] data,int rate){this.data=data;this.rate=rate;}
    private static int u16(RandomAccessFile f)throws IOException{return f.readUnsignedByte()|(f.readUnsignedByte()<<8);}
    private static long u32(RandomAccessFile f)throws IOException{return (long)u16(f)|((long)u16(f)<<16);}
    private static String tag(RandomAccessFile f)throws IOException{byte[] b=new byte[4];f.readFully(b);return new String(b,"US-ASCII");}
    static WavSample read(File file)throws IOException{
        if(file.length()>MAX_BYTES)throw new IOException("WAV must be 32 MiB or smaller");
        try(RandomAccessFile f=new RandomAccessFile(file,"r")){
            if(!tag(f).equals("RIFF"))throw new IOException("Choose an uncompressed RIFF WAV file");
            long end=u32(f)+8;if(end>f.length()||end<12||!tag(f).equals("WAVE"))throw new IOException("Incomplete WAV header");
            int format=0,channels=0,rate=0,bits=0,align=0;long offset=-1,size=0;
            while(f.getFilePointer()+8<=end){
                String id=tag(f);long n=u32(f),start=f.getFilePointer();
                if(n>end-start)throw new IOException("Truncated WAV chunk");
                if(id.equals("fmt ")){
                    if(n<16)throw new IOException("Invalid WAV format");
                    format=u16(f);channels=u16(f);long hz=u32(f);u32(f);align=u16(f);bits=u16(f);
                    if(hz<8000||hz>192000)throw new IOException("Sample rate must be 8–192 kHz");rate=(int)hz;
                }else if(id.equals("data")&&offset<0){offset=start;size=n;}
                f.seek(start+n+(n&1));
            }
            if(offset<0||channels<1||channels>2||align!=channels*((bits+7)/8)||
               !((format==1&&(bits==8||bits==16||bits==24||bits==32))||(format==3&&bits==32)))
                throw new IOException("Use mono/stereo PCM 8/16/24/32-bit or float32 WAV");
            long frames=size/align;
            if(size%align!=0||frames<2||frames>4000000)throw new IOException("WAV must contain 2–4,000,000 frames");
            float[] data=new float[(int)frames];byte[] bytes=new byte[(int)size];f.seek(offset);f.readFully(bytes);
            int p=0,step=bits/8;
            for(int i=0;i<data.length;i++){
                float sum=0;
                for(int c=0;c<channels;c++){
                    int v=bytes[p]&255;if(bits>8)v|=(bytes[p+1]&255)<<8;if(bits>16)v|=(bytes[p+2]&255)<<16;if(bits>24)v|=bytes[p+3]<<24;
                    float x;
                    if(format==3)x=Float.intBitsToFloat(v);
                    else if(bits==8)x=(v-128)/128f;
                    else if(bits==16)x=(short)v/32768f;
                    else if(bits==24)x=(v<<8>>8)/8388608f;
                    else x=v/2147483648f;
                    if(!Float.isFinite(x))x=0;
                    sum+=Math.max(-1,Math.min(1,x));p+=step;
                }
                data[i]=sum/channels;
            }
            return new WavSample(data,rate);
        }catch(EOFException e){throw new IOException("Incomplete WAV file");}
    }
}
