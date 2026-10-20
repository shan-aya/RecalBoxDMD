
(function(){
/* v78 : une page prerendue (ou servie depuis le cache) n'a PAS fait entrer le DMD en mode config : on l'annonce au DMD quand l'utilisateur l'ouvre vraiment. */
(function(){var open=function(){fetch('/dmd-open?msg='+encodeURIComponent('WEB DMD CONFIG'),{method:'POST'}).catch(function(){});};   /* v353 : la route /dmd-open n'accepte que POST (le GET d'avant n'atteignait jamais le DMD : l'accueil tape a la main ne le mettait pas en mode config) */
 if(document.prerendering){document.addEventListener('prerenderingchange',open,{once:true});}
 else{try{var nv=performance.getEntriesByType('navigation')[0];if(nv&&nv.transferSize===0&&nv.type==='navigate')open();}catch(e){}}})();
var P=location.pathname,L='fr';
var M={fr:{ban:'Mode Recalbox natif actif : les fonctions de cette page sont desactivees (l\'ecran est pilote par la Recalbox).',tip:'Inactif en mode Recalbox natif',hint:'Dans la Recalbox, renseignez l\'adresse de ce DMD : dmd.zedmd.wifi={ip} (reglage DMD « ZeDMD WiFi »). Cette adresse ne doit pas changer : donnez une IP fixe a ce DMD (page Wi-Fi, ou reservation DHCP dans votre box).'},
en:{ban:'Native Recalbox mode is on: the features of this page are disabled (the screen is driven by Recalbox).',tip:'Disabled in native Recalbox mode',hint:'In Recalbox, enter this DMD\'s address: dmd.zedmd.wifi={ip} (DMD setting "ZeDMD WiFi"). The address must not change: give this DMD a fixed IP (Wi-Fi page, or a DHCP reservation in your router).'},
es:{ban:'Modo Recalbox nativo activo: las funciones de esta pagina estan desactivadas (la pantalla la controla Recalbox).',tip:'Inactivo en modo Recalbox nativo',hint:'En Recalbox, indica la direccion de este DMD: dmd.zedmd.wifi={ip} (ajuste DMD "ZeDMD WiFi"). La direccion no debe cambiar: asigna una IP fija a este DMD (pagina Wi-Fi, o una reserva DHCP en tu router).'}};
function m(){return M[L]||M.fr;}
function off(el,on){
 if(!el)return;
 if(on){ if(!el.hasAttribute('data-nat'))el.setAttribute('data-nat',el.disabled?'d':'e');
  if('disabled' in el)el.disabled=true; el.style.opacity='.4'; el.style.pointerEvents='none'; el.title=m().tip; }
 else if(el.hasAttribute('data-nat')){ if(el.getAttribute('data-nat')==='e'&&'disabled' in el)el.disabled=false;
  el.style.opacity=''; el.style.pointerEvents=''; el.removeAttribute('data-nat'); el.removeAttribute('title'); }
}
function row(id){var e=document.getElementById(id);return e?(e.closest('.row')||e):null;}
function apply(on){
 var q=document.querySelectorAll('a[href="/config/basic"],a[href="/config/playlist"],a[href="/config/clock"],a[href="/config/media"]');
 for(var i=0;i<q.length;i++)off(q[i],on);
 var nat=document.getElementById('recalbox_native_mode');
 if(nat){ /* page d'accueil (Affichage) */
  /* v75 : les cases sont GRISEES mais gardent leur valeur (plus de decochage force : decocher le mode natif retrouve les reglages d'avant) */
  off(document.getElementById('feat_vpinball_dmd'),on); off(row('feat_vpinball_dmd'),on);
  off(document.getElementById('feat_ra_window'),on); off(row('feat_ra_window'),on);
  var hint=document.getElementById('natHint'), nr=row('recalbox_native_mode');
  if(on&&!hint&&nr){hint=document.createElement('div');hint.id='natHint';hint.style.cssText='font-size:12px;color:#ffedd5;background:#3a2410;border:1px solid #7a4a12;border-radius:6px;padding:8px 10px;margin:6px 0';hint.textContent=m().hint.replace('{ip}',location.hostname);nr.parentNode.insertBefore(hint,nr.nextSibling);}
  if(!on&&hint)hint.parentNode.removeChild(hint);
 }
 if(/^\/config\/(basic|playlist|clock|media)/.test(P)){
  var b=document.getElementById('nativeBanner');
  if(on&&!b){b=document.createElement('div');b.id='nativeBanner';b.style.cssText='background:#7c2d12;color:#ffedd5;padding:10px 12px;border-radius:8px;margin:10px 0;font-size:13px;font-weight:600;text-align:center';b.textContent=m().ban;document.body.insertBefore(b,document.body.firstChild);}
  if(!on&&b)b.parentNode.removeChild(b);
  var c=document.querySelectorAll('input,select,textarea,button');
  for(var j=0;j<c.length;j++)off(c[j],on);
  var s=document.querySelectorAll('.section');
  for(var k=0;k<s.length;k++){s[k].style.opacity=on?'.45':'';}
 }
}
function init(){
 var nat=document.getElementById('recalbox_native_mode');
 if(nat)nat.addEventListener('change',function(){apply(nat.checked);});
 fetch('/lang').then(function(r){return r.json();}).then(function(d){if(d&&d.language)L=String(d.language).slice(0,2);}).catch(function(){})
 .then(function(){return fetch('/load');}).then(function(r){return r.json();})
 .then(function(d){ setTimeout(function(){apply(d.recalbox_native_mode==='1');},300); }).catch(function(){});
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();
