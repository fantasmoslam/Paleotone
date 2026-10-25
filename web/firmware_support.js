// SPDX-License-Identifier: GPL-3.0-only
import {Updater} from './fm1ota.js';
import {MidiLink} from './remote_screen.js';
import {sha256hex} from './fm1pkg.js';

export function classifyFirmware(identity,status,release){
  if(identity.model!=='FM-1')return {kind:'wrong-model',message:'This release is for the FM-1.'};
  if(status?.id===release.rendererId&&status.schema===release.schema)
    return {kind:'ready',message:'Your firmware is ready for the display app. No installation is needed.'};
  const supported=status&&release.supportedBuilds?.find(b=>b.rendererId===status.id&&b.schema===status.schema);
  if(supported)return {kind:'ready',message:supported.name+' is ready for its display app.',displayPath:supported.display};
  if(status&&release.previousRenderers.includes(status.id))
    return {kind:'upgrade',message:'An earlier PALEOTONE display build is installed. Update PALEOTONE to use the current app.'};
  return {kind:'unsupported',message:'This firmware is not a supported display build. Installing PALEOTONE replaces the current firmware; it does not add display support to another firmware.'};
}

// Read-only: identity handshake and display status only. No upgrade command,
// snapshot capture, button input, flash access or automatic installation.
export async function inspectDevice(access,release,{UpdaterClass=Updater,LinkClass=MidiLink}={}){
  const up=new UpdaterClass(access),device=await up.find();
  if(!device)throw new Error('No FM-1 found. Check the USB data cable and disconnect other display/editor pages.');
  const identity=device.id,input=device.link.input,output=device.link.output;device.link.close();
  if(identity.model!=='FM-1')return {...classifyFirmware(identity,null,release),identity};
  const link=new LinkClass(input,output,500);let status=null;
  try{await input.open();await output.open();status=await link.status(0);}
  catch(e){if(e.code!=='MIDI_TIMEOUT')throw e;}
  finally{link.close();}
  return {...classifyFirmware(identity,status,release),identity,status};
}

export async function inspectPackage(bytes,release){
  const hash=await sha256hex(bytes);
  if(hash===release.firmware.sha256)return {kind:'ready',hash,message:'This is the current app-compatible PALEOTONE package.'};
  const supported=release.supportedBuilds?.find(b=>b.firmwareSha256===hash);
  if(supported)return {kind:'ready',hash,message:'This is the app-compatible '+supported.name+' package.'};
  if(release.previousPackages.includes(hash))return {kind:'upgrade',hash,message:'This is an earlier tested PALEOTONE package. Use the current PALEOTONE installer to update.'};
  const patch=release.patchSources?.find(b=>b.sha256===hash);
  if(patch)return {kind:'patch',hash,message:patch.name+' is supported. Use the GHOULBOX package patcher above to add the display app.'};
  return {kind:'unsupported',hash,message:'This package is not a supported build. It has not been modified. Supporting it requires its source and a matching display renderer.'};
}
