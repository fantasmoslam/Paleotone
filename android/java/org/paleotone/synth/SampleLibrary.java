/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;
import android.content.*;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import java.io.*;
import java.security.*;
import java.util.*;
final class SampleLibrary {
    final Context context;final File directory;
    static final class Entry {final File file;final String name;Entry(File f,String n){file=f;name=n;}String id(){return file.getName();}}
    SampleLibrary(Context c){context=c;directory=new File(c.getFilesDir(),"sample-library");}
    private void ready()throws IOException{if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Cannot create sample library");}
    void starters()throws IOException{
        ready();if(new File(directory,".starter-v1").exists())return;
        for(String name:new String[]{"Flute C4.wav","Jungle C3.wav"}){
            try(InputStream in=context.getAssets().open("starter/"+name)){store(in,name);}
        }
        try(FileOutputStream out=new FileOutputStream(new File(directory,".starter-v1"))){out.write(1);}
    }
    List<Entry> list(){
        List<Entry> result=new ArrayList<>();File[] files=directory.listFiles();if(files==null)return result;
        for(File file:files)if(file.getName().endsWith(".wav")){
            String name=file.getName();File label=new File(directory,name+".name");
            try(DataInputStream in=new DataInputStream(new FileInputStream(label))){name=in.readUTF();}catch(IOException ignored){}
            result.add(new Entry(file,name));
        }
        Collections.sort(result,(a,b)->a.name.compareToIgnoreCase(b.name));return result;
    }
    Entry find(String id){for(Entry e:list())if(e.id().equals(id))return e;return null;}
    Entry importUri(Uri uri)throws IOException{
        String name="Imported WAV";
        try(Cursor c=context.getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)){
            if(c!=null&&c.moveToFirst()&&!c.isNull(0))name=c.getString(0);
        }
        try(InputStream in=context.getContentResolver().openInputStream(uri)){
            if(in==null)throw new IOException("Cannot open selected file");return store(in,name);
        }
    }
    private Entry store(InputStream in,String name)throws IOException{
        ready();File temp=File.createTempFile("import-",".part",directory);
        try{
            MessageDigest hash;try{hash=MessageDigest.getInstance("SHA-256");}catch(NoSuchAlgorithmException e){throw new IOException(e);}
            try(OutputStream out=new FileOutputStream(temp)){
                byte[] b=new byte[65536];int n;long count=0;
                while((n=in.read(b))!=-1){count+=n;if(count>WavSample.MAX_BYTES)throw new IOException("WAV must be 32 MiB or smaller");out.write(b,0,n);hash.update(b,0,n);}
            }
            WavSample.read(temp); // Validate before committing to the library.
            StringBuilder key=new StringBuilder();for(byte b:hash.digest())key.append(String.format(Locale.ROOT,"%02x",b&255));
            File target=new File(directory,key+".wav");
            if(!target.exists()&&!temp.renameTo(target))throw new IOException("Cannot save sample");
            name=name.replaceAll("[\\p{Cntrl}]", " ");if(name.length()>120)name=name.substring(0,120);
            File label=new File(directory,target.getName()+".name");
            if(!label.exists())try(DataOutputStream out=new DataOutputStream(new FileOutputStream(label))){out.writeUTF(name);}
            return new Entry(target,name);
        }finally{temp.delete();}
    }
    void delete(Entry e)throws IOException{
        if(!e.file.delete())throw new IOException("Cannot delete sample");new File(directory,e.id()+".name").delete();
    }
}
