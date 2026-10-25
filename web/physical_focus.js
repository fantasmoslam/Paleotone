// SPDX-License-Identifier: GPL-3.0-only
// Hardware events are absolute targets; never replay a consumed event after a timeout.
export class PhysicalFocus {
  constructor(link,select){this.link=link;this.select=select;this.serial=null;this.enabled=false;}
  async poll(enabled){
    const p=await this.link.request(96,[enabled?1:0],p=>p.length===5&&p[0]===1&&p[1]<=1&&p[4]<=1);
    const serial=p[2]|p[3]<<7;
    if(!enabled||!this.enabled||this.serial===null){this.serial=serial;this.enabled=enabled;return;}
    if(serial===this.serial||p[4])return; // wait until HOME and OCT are both released
    this.serial=serial;
    await this.select(p[1]?'right':'left');
  }
}
