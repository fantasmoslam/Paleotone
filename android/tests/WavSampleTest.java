/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;
import java.io.*;
import java.nio.*;
final class WavSampleTest {
    static File fixture(int format,int bits,int channels,int... values)throws IOException{
        ByteBuffer b=ByteBuffer.allocate(44+values.length*bits/8).order(ByteOrder.LITTLE_ENDIAN);
        b.put("RIFF".getBytes("US-ASCII")).putInt(b.capacity()-8).put("WAVEfmt ".getBytes("US-ASCII")).putInt(16);
        b.putShort((short)format).putShort((short)channels).putInt(48000).putInt(48000*channels*bits/8).putShort((short)(channels*bits/8)).putShort((short)bits);
        b.put("data".getBytes("US-ASCII")).putInt(values.length*bits/8);for(int v:values)for(int i=0;i<bits/8;i++)b.put((byte)(v>>(8*i)));
        File f=File.createTempFile("paleotone-wave-",".wav");f.deleteOnExit();try(FileOutputStream o=new FileOutputStream(f)){o.write(b.array());}return f;
    }
    static void close(float a,float b){assert Math.abs(a-b)<.00001:a+" != "+b;}
    static void bad(File f)throws Exception{try{WavSample.read(f);throw new AssertionError("Accepted invalid WAV");}catch(IOException expected){}}
    public static void main(String[] args)throws Exception{
        WavSample w=WavSample.read(fixture(1,16,2,32767,-32768,16384,16384));assert w.rate==48000&&w.data.length==2;close(w.data[0],-1/65536f);close(w.data[1],.5f);
        w=WavSample.read(fixture(1,24,1,-8388608,4194304));close(w.data[0],-1);close(w.data[1],.5f);
        w=WavSample.read(fixture(1,32,1,Integer.MIN_VALUE,1073741824));close(w.data[0],-1);close(w.data[1],.5f);
        w=WavSample.read(fixture(1,8,1,0,255));close(w.data[0],-1);close(w.data[1],127/128f);
        w=WavSample.read(fixture(3,32,1,Float.floatToIntBits(Float.NaN),Float.floatToIntBits(9)));close(w.data[0],0);close(w.data[1],1);
        bad(fixture(6,16,1,0,0));bad(fixture(1,16,1,0));bad(fixture(1,16,3,0,0,0,0,0,0));
        File truncated=fixture(1,16,1,0,1);try(RandomAccessFile f=new RandomAccessFile(truncated,"rw")){f.setLength(45);}bad(truncated);
        for(String path:args){w=WavSample.read(new File(path));assert w.data.length>2;float peak=0;for(float x:w.data)peak=Math.max(peak,Math.abs(x));assert peak>.5f&&peak<=.751f;}
        System.out.println("PASS: WAV PCM 8/16/24/32, float32 sanitation, stereo downmix, malformed/truncated rejection and bundled samples");
    }
}
