// SPDX-License-Identifier: GPL-3.0-only
import {Package,sha256hex,productOf} from './fm1pkg.js';
export async function patchSupportedPackage(original,app,recipe){
  if(original.length!==recipe.input.size||await sha256hex(original)!==recipe.input.sha256)
    throw new Error('This is not the exact supported original firmware package. No patch was applied.');
  if(app.length!==recipe.app.size||await sha256hex(app)!==recipe.app.sha256)
    throw new Error('The display integration download failed verification.');
  if(recipe.product!=='FM-1_900'||productOf(original)!==recipe.product)throw new Error('Wrong device model.');
  const pkg=new Package(original);
  if(app.length>pkg.app.size-16384)throw new Error('The display build exceeds the reserved application space.');
  pkg.setApp(app);const output=pkg.serialize();
  if(await sha256hex(output)!==recipe.output.sha256)throw new Error('The patched package failed verification.');
  const checked=new Package(output);
  if(checked.product!==recipe.product||output.length!==recipe.output.size)throw new Error('Invalid patched package.');
  return output;
}
