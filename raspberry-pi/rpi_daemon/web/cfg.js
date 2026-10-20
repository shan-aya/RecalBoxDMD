/* cfg.js -- code partage des pages web config (DECISIONS.md 3.20). Charge en <head>, AVANT le script de la page.
   FILE D'ATTENTE DES REQUETES : le serveur du DMD ne traite qu'UNE connexion a la fois. Chrome en ouvre jusqu'a 6 en parallele
   (/lang, /load, listes...) : mesure sur DMD1, 6 requetes simultanees = 13 echecs sur 30, 1 seule = 0 sur 5. On passe donc les
   requetes GET (et les messages POST /dmd-pause) UNE PAR UNE, avec jusqu'a 8 tentatives espacees de 1,2 s pour les GET (erreur reseau) : le DMD a des episodes de 15 a 20 s ou il n'emet plus (DECISIONS.md 3.20).
   Les autres requetes (envois de fichiers, /save, /reboot...) ne sont pas touchees. */
try { sessionStorage.removeItem('cfgR'); } catch (e) {}   // cfg.js bien arrive : on re-arme le rechargement de securite
(function () {
  var of = window.fetch.bind(window), q = [], busy = false;
  /* AVIS DE MISE A JOUR (v347-v348) : barre affichee en haut de CHAQUE page (accueil comprise), meme si la mise a jour se lance depuis la page Reglages. Deux avis :
       - FIRMWARE : version installee (fw_version de la reponse /load) < version publiee (manifeste GitHub, champ version) -> lien vers /config/settings (bouton OTA) ;
       - SCRIPTS RECALBOX / TOOLKIT : lot de scripts installe (scripts_bundle de /load, ou 0 si la Recalbox parle depuis >= 90 s sans l'annoncer) < lot publie (champ scripts) ->
         lien vers le toolkit (releases GitHub) : c'est lui qui installe les scripts (Mode 9) ; la version du toolkit installe sur le PC n'est pas visible du DMD.
     Les donnees DMD sont lues au passage de la reponse /load (aucune requete de plus) ; le manifeste est interroge par le NAVIGATEUR au plus toutes les 5 min (localStorage ; 6 h jusqu en v354 : un firmware publie n etait annonce qu apres des heures) ; hors ligne : silence. */
  var FWK = 'dmd_fw_manifest', FWT = 'dmd_fw_manifest_t', SCK = 'dmd_scr_manifest', TKK = 'dmd_tk_manifest', TKS = 'dmd_tk_seen';
  function vnum(s) { var m = /^(\d+)\.(\d+)$/.exec(s || ''); return m ? parseInt(m[1], 10) * 100 + parseInt(m[2], 10) : 0; }
  var TXT = {
    fw: { fr: 'Mise à jour du firmware disponible : v{v} (v{c} installée) — ouvrir Réglages', en: 'Firmware update available: v{v} (v{c} installed) — open Settings', es: 'Actualización de firmware disponible: v{v} (v{c} instalada) — abrir Ajustes' },
    sc: { fr: 'Mise à jour des scripts Recalbox disponible : lot {v} (lot {c} installé) — télécharger le toolkit puis lancer le Mode 9', en: 'Recalbox scripts update available: bundle {v} (bundle {c} installed) — download the toolkit, then run Mode 9', es: 'Actualización de los scripts de Recalbox disponible: lote {v} (lote {c} instalado) — descargar el toolkit y ejecutar el Modo 9' },
    tk: { fr: 'Dernier toolkit PC publié : build {v} — à télécharger si le vôtre est plus ancien', en: 'Latest PC toolkit published: build {v} — download it if yours is older', es: 'Última caja de herramientas del PC publicada: build {v} — descárguela si la suya es anterior' }   /* le DMD ne connait pas la version du toolkit installe sur le PC : avis une fois par build publie, a fermer */
  };
  /* v353 : lien vers les notes de version (CHANGELOG dans la langue de la page) a cote de chaque avis de mise a jour ; l'adresse s'affiche aussi au survol */
  var CL = { fr: ['Notes de version', '.fr'], en: ['Release notes', ''], es: ['Notas de versión', '.es'] };
  function clLang() { var l = (localStorage.getItem('dmd_lang') || document.documentElement.lang || 'fr').substring(0, 2); return CL[l] ? l : 'fr'; }
  function clUrl() { return 'https://github.com/shan-aya/RecalBoxDMD/blob/main/CHANGELOG' + CL[clLang()][1] + '.md'; }
  window.cfgNotes = function () { return '<a class="cfgcl" href="' + clUrl() + '" target="_blank" rel="noopener">' + CL[clLang()][0] + '</a>'; };
  /* survol : le TEXTE du changelog (dernieres entrees, mise en forme retiree) s'affiche dans une bulle ; lu une fois sur GitHub (raw) par le navigateur, hors ligne : silence */
  var clTxt = {}, clTip = null;
  function clParse(md) {
    var parts = md.replace(/\r/g, '').split(/^## /m).slice(1), out = '', i;
    for (i = 0; i < parts.length && out.length < 1300; i++) {
      var s = parts[i].replace(/\[([^\]]*)\]\([^)]*\)/g, '$1').replace(/\*\*|`|\*/g, '').replace(/^- /gm, '• ').replace(/\n{3,}/g, '\n\n').trim();
      out += (out ? '\n\n' : '') + s;
    }
    return out.length > 1500 ? out.substring(0, 1500).replace(/\s+\S*$/, '') + ' …' : out;
  }
  function clShow(el) {
    var lg = clLang();
    var put = function (t) {
      if (!clTip) { clTip = document.createElement('div'); clTip.style.cssText = 'position:fixed;z-index:100001;max-width:min(560px,92vw);max-height:70vh;overflow:hidden;background:#101a33;color:#eee;border:1px solid #8ab4f8;border-radius:8px;padding:10px 12px;font:12px/1.45 "Segoe UI",Tahoma,sans-serif;white-space:pre-wrap;text-align:left;pointer-events:none;box-shadow:0 4px 18px rgba(0,0,0,.5)'; document.body.appendChild(clTip); }
      clTip.textContent = t; clTip.style.display = 'block';
      var r = el.getBoundingClientRect(), w = clTip.offsetWidth, h = clTip.offsetHeight;
      clTip.style.left = Math.max(8, Math.min(r.left, window.innerWidth - w - 8)) + 'px';
      clTip.style.top = (r.bottom + h + 12 < window.innerHeight ? r.bottom + 6 : Math.max(8, r.top - h - 6)) + 'px';
    };
    if (clTxt[lg]) { put(clTxt[lg]); return; }
    put('…');
    fetch('https://raw.githubusercontent.com/shan-aya/RecalBoxDMD/main/CHANGELOG' + CL[lg][1] + '.md').then(function (r) { return r.text(); }).then(function (m) { clTxt[lg] = clParse(m); if (clTip && clTip.style.display === 'block') put(clTxt[lg]); }).catch(function () { if (clTip) clTip.style.display = 'none'; });
  }
  document.addEventListener('mouseover', function (e) { var t = e.target && e.target.closest ? e.target.closest('.cfgcl') : null; if (t) clShow(t); });
  document.addEventListener('mouseout', function (e) { var t = e.target && e.target.closest ? e.target.closest('.cfgcl') : null; if (t && clTip) clTip.style.display = 'none'; });
  function updBar(d, latest, scrLatest, tkLatest) {
    var onSet = location.pathname === '/config/settings';   /* la page Reglages a ses propres avis firmware / scripts : seul l'avis toolkit s'y ajoute */
    var lg = (localStorage.getItem('dmd_lang') || document.documentElement.lang || 'fr').substring(0, 2); if (!TXT.fw[lg]) lg = 'fr';
    var cur = String((d && d.fw_version) || ''), items = [];
    var fwNew = vnum(cur) > 0 && vnum(latest) > vnum(cur);
    if (!onSet && fwNew) items.push({ t: TXT.fw[lg].replace('{v}', latest).replace('{c}', cur), h: '/config/settings', b: false });
    var sl = parseInt(scrLatest, 10) || 0, sc = String((d && d.scripts_bundle) || ''), age = parseInt((d && d.rb_age) || '0', 10) || 0;
    var have = sc !== '' ? parseInt(sc, 10) : (age >= 90 ? 0 : -1);
    /* v356 : pastille « Mise a jour » sur chaque lien vers Reglages (bouton de l'accueil, menu des pages) : pseudo-element CSS (texte selon html[lang]) : survit au changement de langue (innerHTML reecrit) */
    if (!onSet && (fwNew || (sl > 0 && have >= 0 && have < sl))) {
      var badge = function () {
        if (!document.getElementById('cfgBadgeCss')) { var st = document.createElement('style'); st.id = 'cfgBadgeCss'; st.textContent = 'a[data-upd]::after{content:"\\2B06  Mise \\E0  jour";display:inline-block;margin-left:8px;padding:1px 7px;border-radius:10px;background:#ffb04a;color:#3a2200;font:700 11px "Segoe UI",Tahoma,sans-serif;white-space:nowrap;vertical-align:middle}html[lang=en] a[data-upd]::after{content:"\\2B06  Update"}html[lang=es] a[data-upd]::after{content:"\\2B06  Actualizaci\\F3 n"}'; document.head.appendChild(st); }
        var ls = document.querySelectorAll('a[href="/config/settings"]'); for (var i = 0; i < ls.length; i++) if (!ls[i].closest('#cfgFwBar')) ls[i].setAttribute('data-upd', '1');
      };
      if (document.body) badge(); else document.addEventListener('DOMContentLoaded', badge);
    }
    if (!onSet && sl > 0 && have >= 0 && have < sl) items.push({ t: TXT.sc[lg].replace('{v}', sl).replace('{c}', have), h: 'https://github.com/shan-aya/RecalBoxDMD/releases/latest', b: true });
    var tkn = parseInt(tkLatest, 10) || 0, tks = 0; try { tks = parseInt(localStorage.getItem(TKS) || '0', 10) || 0; } catch (e) {}
    if (tkn > 0 && tkn > tks) items.push({ t: TXT.tk[lg].replace('{v}', tkn), h: 'https://github.com/shan-aya/RecalBoxDMD/releases/latest', b: true, tk: tkn });
    if (!items.length) return;
    var show = function () {
      if (document.getElementById('cfgFwBar') || !document.body) return;
      var w = document.createElement('div'); w.id = 'cfgFwBar'; w.style.cssText = 'margin:0 0 10px';
      items.forEach(function (it) {
        var r = document.createElement('div'), a = document.createElement('a'), n = document.createElement('a');
        a.href = it.h; if (it.b) { a.target = '_blank'; a.rel = 'noopener'; } a.textContent = it.t; a.style.cssText = 'color:inherit;text-decoration:none';
        n.href = clUrl(); n.target = '_blank'; n.rel = 'noopener'; n.className = 'cfgcl'; n.textContent = CL[clLang()][0]; n.style.cssText = 'color:inherit;text-decoration:underline;display:block;margin-top:3px';
        r.style.cssText = 'text-align:center;background:#7a4a12;color:#ffcf8a;font:700 13px "Segoe UI",Tahoma,sans-serif;padding:8px 10px;margin:0 0 4px;border-radius:8px';
        r.appendChild(a); r.appendChild(n); w.appendChild(r);
        if (it.tk) { var x = document.createElement('span'); x.textContent = '\u2715'; x.title = 'OK'; x.style.cssText = 'float:left;cursor:pointer;padding:0 4px;margin:-2px 6px 0 0'; x.onclick = function () { try { localStorage.setItem(TKS, String(it.tk)); } catch (e) {} r.parentNode && r.parentNode.removeChild(r); }; r.insertBefore(x, r.firstChild); }
      });
      document.body.insertBefore(w, document.body.firstChild);
    };
    if (document.body) show(); else document.addEventListener('DOMContentLoaded', show);
  }
  function fwCheck(d) {
    if (!d || !d.fw_version) return;
    var now = Date.now(), t = 0, latest = '', scr = '', tk = '';
    try { t = parseInt(localStorage.getItem(FWT) || '0', 10) || 0; latest = localStorage.getItem(FWK) || ''; scr = localStorage.getItem(SCK) || ''; tk = localStorage.getItem(TKK) || ''; } catch (e) {}
    if (latest && now - t < 5 * 60 * 1000) { updBar(d, latest, scr, tk); return; }
    var c = new AbortController(), to = setTimeout(function () { c.abort(); }, 6000);
    of('https://shan-aya.github.io/RecalBoxDMD/install/manifest.json', { cache: 'no-store', signal: c.signal }).then(function (r) { return r.json(); }).then(function (m) {
      latest = String((m && m.version) || ''); scr = String((m && m.scripts) || ''); tk = String((m && m.toolkit) || '');
      try { localStorage.setItem(FWK, latest); localStorage.setItem(SCK, scr); localStorage.setItem(TKK, tk); localStorage.setItem(FWT, String(now)); } catch (e) {}
      updBar(d, latest, scr, tk);
    }).catch(function () { if (latest) updBar(d, latest, scr, tk); }).then(function () { clearTimeout(to); });
  }
  function abortErr() { try { return new DOMException('Aborted', 'AbortError'); } catch (e) { var x = new Error('Aborted'); x.name = 'AbortError'; return x; } }
  function pump() {
    if (busy) return;
    var j = q.shift(); if (!j) return;
    if (j.o.signal && j.o.signal.aborted) { j.rej(abortErr()); return pump(); }
    busy = true; go(j, 0);
  }
  function fin() { busy = false; pump(); }
  function go(j, n) {
    of(j.u, j.o).then(function (r) {
      if (typeof j.u === 'string' && /^\/load(\?|$)/.test(j.u)) { try { r.clone().json().then(function (d) { fwCheck(d); }).catch(function () {}); } catch (e) {} }
      j.res(r); fin();
    }, function (e) {
      var ab = (e && e.name === 'AbortError') || (j.o.signal && j.o.signal.aborted);
      if (j.retry && !ab && n < 7) { setTimeout(function () { go(j, n + 1); }, 1200); return; }
      j.rej(e); fin();
    });
  }
  window.fetch = function (u, o) {
    o = o || {};
    var url = typeof u === 'string' ? u : (u && u.url) || '';
    if (/^https?:/i.test(url) && url.indexOf(location.origin) !== 0) return of(u, o);   // hors DMD (meteo, GitHub...) : tel quel
    var m = (o.method || 'GET').toUpperCase();
    var queued = (m === 'GET' || m === 'HEAD') || (m === 'POST' && url.indexOf('/dmd-pause') === 0);
    if (!queued) return of(u, o);
    return new Promise(function (res, rej) {
      var j = { u: u, o: o, res: res, rej: rej, retry: (m === 'GET' || m === 'HEAD') };
      if (o.signal) o.signal.addEventListener('abort', function () { var i = q.indexOf(j); if (i >= 0) { q.splice(i, 1); rej(abortErr()); } });
      q.push(j); pump();
    });
  };
})();

/* v352 -- BROUILLON PARTAGE (pages Widgets, Lieu, Evenements, Couleurs d'horloge + fusion des brouillons de TOUTES les pages) : les reglages faits dans un sous-menu sont gardes
   dans localStorage (dmd_draft_widgets, seulement les cles MODIFIEES) et envoyes par UN SEUL « Enregistrer » de n'importe quelle page (wdBodyAll). Effaces apres un enregistrement reussi. */
var WD_KEY = 'dmd_draft_widgets', WD_ALL = ['dmd_draft_home', 'dmd_draft_basic', 'dmd_draft_playlist', 'dmd_draft_network', 'dmd_draft_clock', 'dmd_draft_widgets'];
function wdGet() { try { return JSON.parse(localStorage.getItem(WD_KEY) || '{}') || {}; } catch (e) { return {}; } }
function wdPatch(o, del) { try { var c = wdGet(); Object.keys(o || {}).forEach(function (k) { c[k] = o[k]; }); (del || []).forEach(function (k) { delete c[k]; }); if (Object.keys(c).length) localStorage.setItem(WD_KEY, JSON.stringify(c)); else localStorage.removeItem(WD_KEY); } catch (e) {} }
function wdClearAll() { WD_ALL.forEach(function (k) { try { localStorage.removeItem(k); } catch (e) {} }); }
function wdBodyAll(own) {
  var p = new URLSearchParams();
  WD_ALL.forEach(function (k) { try { var raw = localStorage.getItem(k); if (!raw) return; var o = JSON.parse(raw);
    Object.keys(o).forEach(function (id) { var v = o[id]; if ((v === '' && !/^(clk_anim_|clk_digit_|deco_ev_)/.test(id)) || v === undefined || v === null) return; if (/^(wifi_|bluetooth_)/.test(id)) return; p.set(id, (typeof v === 'boolean') ? (v ? '1' : '0') : String(v)); }); } catch (e) {} });
  if (own) own.forEach(function (v, k) { p.set(k, v); });
  return p;
}
/* v353 : boutons de l'accueil et des Reglages (pas de formulaire propre) : reprendre le DMD ; enregistrer les brouillons des autres pages puis redemarrer */
function cfgMsg(t, ok) { if (typeof showMsgLocal === 'function') showMsgLocal(t, ok); }
function cfgResume() { fetch('/dmd-resume', { method: 'POST' }).then(function () { cfgMsg(tr('msg_dmd_resumed'), true); }).catch(function () {}); }
function cfgSaveReboot() {
  var b = wdBodyAll(), n = 0; b.forEach(function () { n++; });
  var go = function () { cfgMsg(tr('msg_rebooting'), true); fetch('/reboot').catch(function () {}); };
  if (!n) { go(); return; }
  fetch('/save', { method: 'POST', body: b, headers: { 'Content-Type': 'application/x-www-form-urlencoded' } }).then(function (r) { return r.text(); }).then(function (x) { if (x.indexOf('OK') >= 0) { wdClearAll(); setTimeout(go, 400); } else cfgMsg(x, false); }).catch(function () {});
}

/* v357 : Raspberry Pi -- /reboot repond « DAEMON » (demon relance) ou « PIREBOOT » (Pi redemarre) : on attend le retour du serveur puis une fenetre l'annonce. Sur l'ESP32 la reponse est « REBOOT » : aucun effet. */
(function(){
  const of=window.fetch;
  const T={fr:{d:'Daemon redémarré avec succès',p:'Raspberry Pi redémarré avec succès',ok:'OK'},en:{d:'Daemon restarted successfully',p:'Raspberry Pi restarted successfully',ok:'OK'},es:{d:'Demonio reiniciado con éxito',p:'Raspberry Pi reiniciado con éxito',ok:'OK'}};
  function popup(txt,ok){
    const ov=document.createElement('div');ov.style.cssText='position:fixed;left:0;top:0;right:0;bottom:0;z-index:100000;background:rgba(0,0,0,.65);display:flex;align-items:center;justify-content:center;padding:16px';
    const bx=document.createElement('div');bx.style.cssText='background:#1a1a2e;color:#fff;border:2px solid #2d6a4f;border-radius:10px;padding:20px 24px;max-width:420px;width:100%;text-align:center;font-size:16px;font-weight:bold';
    const m=document.createElement('div');m.textContent='\u2705 '+txt;m.style.marginBottom='14px';bx.appendChild(m);
    const b=document.createElement('button');b.type='button';b.textContent=ok;b.style.cssText='cursor:pointer;border:0;border-radius:6px;padding:8px 22px;font-size:14px;font-weight:bold;background:#ffd146;color:#1a1a2e';b.onclick=function(){ov.remove();};
    bx.appendChild(b);ov.appendChild(bx);document.body.appendChild(ov);setTimeout(function(){if(ov.parentNode)ov.remove();},15000);
  }
  function back(kind){
    const L=T[localStorage.getItem('dmd_lang')||document.documentElement.lang]||T.fr;
    const t0=Date.now();let down=false;
    (function poll(){
      of('/lang',{cache:'no-store'}).then(function(r){return r.ok;}).catch(function(){return false;}).then(function(ok){
        if(!ok)down=true;
        if(ok&&(down||Date.now()-t0>4000)){popup(kind==='p'?L.p:L.d,L.ok);return;}
        if(Date.now()-t0<240000)setTimeout(poll,1500);
      });
    })();
  }
  window.fetch=function(u,o){
    const p=of.apply(this,arguments);
    try{if(String(u).indexOf('/reboot')===0){p.then(function(r){return r.clone().text();}).then(function(t){if(t.indexOf('PIREBOOT')===0)back('p');else if(t.indexOf('DAEMON')===0)back('d');}).catch(function(){});}}catch(e){}
    return p;
  };
})();

/* cfg_help.js -- aide COMMUNE des pages web config (genere par tools/web_shared_step3b_help.py, DECISIONS.md 3.20). */
window.CFG_HELP={
fr:{help_link:'Aide',help_title:'Bienvenue sur la configuration du DMD',help_intro:'Par défaut, le DMD affiche des widgets et une horloge aléatoire : rien n\'est obligatoire. Voici de quoi le personnaliser, et un résumé de cette interface.',help_checklist_title:'Pour aller plus loin (facultatif)',help_check_ip:'IP de votre Recalbox (page Wi-Fi) — seulement si vous en utilisez une',help_check_playlist:'Une playlist de GIFs (page Playlist) — la choisir active automatiquement le fond « Playlist de GIFs » ; sans playlist, les widgets tournent seuls',help_check_wifi:'Le Wi-Fi est déjà validé à ce stade — inutile d\'y retoucher, sauf si vous voulez le changer',help_url_reminder:'Cette page reste accessible à tout moment en tapant l\'IP du DMD dans un navigateur — actuellement {ip}',help_features_title:'Ce que permet cette interface',help_feat_playlists:'GIFs (page Médias) : ajouter des GIFs sur la carte SD (upload direct depuis le navigateur, création de dossiers) — les playlists qui référencent un dossier modifié sont mises à jour automatiquement',help_feat_clock:'Widgets (page Widgets) : horloge, date, météo, éphéméride, compte à rebours, citations, ambiances, widgets Recalbox ; fond (widgets seuls ou playlist de GIFs), déclencheur, durées. Horloge : thème, couleurs, fuseau horaire',help_feat_display:'Réglages (page Réglages) : luminosité, démarrage silencieux, mode Pinball, mode Recalbox natif, mise à jour du firmware. Affichage : hi-score/infos/RetroAchievements. Playlist : choix de la playlist, création/suppression à partir des dossiers de GIFs',help_feat_network:'Réseau (page Wi-Fi) : IP de la Recalbox (jusqu\'à 4, le DMD bascule seul sur celle qui est allumée), Wi-Fi (réseau, mot de passe, IP statique)'},
en:{help_link:'Help',help_title:'Welcome to the DMD configuration',help_intro:'By default the DMD shows widgets and a random clock: nothing is mandatory. Here is how to customise it, and a summary of this interface.',help_checklist_title:'To go further (optional)',help_check_ip:'Your Recalbox IP (Wi-Fi page) — only if you use one',help_check_playlist:'A GIF playlist (Playlist page) — choosing one automatically sets the background to “GIF playlist”; without a playlist the widgets run on their own',help_check_wifi:'Wi-Fi is already validated at this stage — no need to touch it again, unless you want to change it',help_url_reminder:'This page stays accessible at any time by typing the DMD\'s IP in a browser — currently {ip}',help_features_title:'What this interface lets you do',help_feat_playlists:'GIFs (Media page): add GIFs to the SD card (direct upload from the browser, folder creation) — playlists referencing a modified folder are updated automatically',help_feat_clock:'Widgets (Widgets page): clock, date, weather, ephemeris, countdown, quotes, ambiences, Recalbox widgets; background (widgets only or GIF playlist), trigger, durations. Clock: theme, colours, time zone',help_feat_display:'Settings (Settings page): brightness, silent startup, Pinball mode, native Recalbox mode, firmware update. Display: hi-score/info/RetroAchievements. Playlist: playlist choice, creating/deleting playlists from GIF folders',help_feat_network:'Network (Wi-Fi page): Recalbox IP (up to 4, the DMD switches by itself to the one that is on), Wi-Fi (network, password, static IP)'},
es:{help_link:'Ayuda',help_title:'Bienvenido a la configuración del DMD',help_intro:'Por defecto, el DMD muestra widgets y un reloj aleatorio: nada es obligatorio. Así puede personalizarlo, y un resumen de esta interfaz.',help_checklist_title:'Para ir más lejos (opcional)',help_check_ip:'IP de su Recalbox (página Wi-Fi) — solo si usa una',help_check_playlist:'Una lista de GIFs (página Playlist) — elegirla activa automáticamente el fondo « Lista de GIFs »; sin lista, los widgets funcionan solos',help_check_wifi:'El Wi-Fi ya está validado en esta etapa — no hace falta tocarlo, salvo que quiera cambiarlo',help_url_reminder:'Esta página sigue accesible en cualquier momento escribiendo la IP del DMD en un navegador — actualmente {ip}',help_features_title:'Qué permite esta interfaz',help_feat_playlists:'GIFs (página Medios): añadir GIFs a la tarjeta SD (subida directa desde el navegador, creación de carpetas) — las playlists que referencian una carpeta modificada se actualizan automáticamente',help_feat_clock:'Widgets (página Widgets): reloj, fecha, tiempo, efemérides, cuenta atrás, citas, ambientes, widgets Recalbox; fondo (solo widgets o lista de GIFs), disparador, duraciones. Reloj: tema, colores, zona horaria',help_feat_display:'Ajustes (página Ajustes): brillo, inicio silencioso, modo Pinball, modo Recalbox nativo, actualización del firmware. Pantalla: hi-score/info/RetroAchievements. Playlist: elección de la lista, creación/eliminación a partir de las carpetas de GIFs',help_feat_network:'Red (página Wi-Fi): IP de Recalbox (hasta 4, el DMD cambia solo a la encendida), Wi-Fi (red, contraseña, IP estática)'}
};;
function showHelpModal(){
  document.getElementById('helpBackdrop').classList.add('show');
  const p=document.getElementById('helpUrlReminder');
  if(p) p.textContent=((CFG_HELP[currentLang]&&CFG_HELP[currentLang].help_url_reminder)||CFG_HELP.fr.help_url_reminder).replace('{ip}',window.location.host);
}
function closeHelpModal(){document.getElementById('helpBackdrop').classList.remove('show');}

/* cfg_i18n.js -- traductions COMMUNES aux pages web config (genere par tools/web_shared_step3_i18n.py, DECISIONS.md 3.20). Les pages gardent la priorite (tr()). */
window.CFG_I18N={
fr:{btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',msg_saving:'Enregistrement...',saving:'Enregistrement...',msg_net_error:'Erreur réseau',essential_wifi:'Wi-Fi',essential_playlist:'Playlist par défaut',essential_ip:'IP Recalbox',msg_essential_missing:'Attention : champ(s) essentiel(s) vide(s) : {fields}. Le DMD risque de ne pas fonctionner correctement. Continuer quand même ?',loading_text:'Chargement en cours...',nav_basic:'&#x1F4A1; Affichage',nav_playlist:'&#x1F4BF; Playlist',nav_network:'&#x1F4F6; Wi-Fi',nav_clock:'&#x23F0; Horloge',nav_deco:'&#x2728; Widgets &amp; Horloge',nav_button:'&#x1F518; Bouton',nav_media:'&#x1F4BF; Médias',nav_home:'&#x1F3E0; Accueil',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_dmd_resumed:'DMD repris',msg_load_error:'Impossible de charger la config',save:'&#x1F4BE; Enregistrer',resume:'&#x25B6; Reprendre DMD',saved:'Enregistré',save_err:'Erreur d\'enregistrement',resumed:'DMD repris',loading:'Chargement en cours...'},
en:{btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',msg_saving:'Saving...',saving:'Saving...',msg_net_error:'Network error',essential_wifi:'Wi-Fi',essential_playlist:'Default playlist',essential_ip:'Recalbox IP',msg_essential_missing:'Warning: missing essential field(s): {fields}. The DMD may not work correctly. Continue anyway?',loading_text:'Loading...',nav_basic:'&#x1F4A1; Display',nav_playlist:'&#x1F4BF; Playlist',nav_network:'&#x1F4F6; Wi-Fi',nav_clock:'&#x23F0; Clock',nav_deco:'&#x2728; Widgets &amp; Clock',nav_button:'&#x1F518; Button',nav_media:'&#x1F4BF; Media',nav_home:'&#x1F3E0; Home',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_dmd_resumed:'DMD resumed',msg_load_error:'Unable to load config',save:'&#x1F4BE; Save',resume:'&#x25B6; Resume DMD',saved:'Saved',save_err:'Save error',resumed:'DMD resumed',loading:'Loading...'},
es:{btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',msg_saving:'Guardando...',saving:'Guardando...',msg_net_error:'Error de red',essential_wifi:'Wi-Fi',essential_playlist:'Playlist por defecto',essential_ip:'IP de Recalbox',msg_essential_missing:'Atención: falta(n) campo(s) esencial(es): {fields}. Es posible que el DMD no funcione correctamente. ¿Continuar de todos modos?',loading_text:'Cargando...',nav_basic:'&#x1F4A1; Pantalla',nav_playlist:'&#x1F4BF; Playlist',nav_network:'&#x1F4F6; Wi-Fi',nav_clock:'&#x23F0; Reloj',nav_deco:'&#x2728; Widgets y Reloj',nav_button:'&#x1F518; Botón',nav_media:'&#x1F4BF; Medios',nav_home:'&#x1F3E0; Inicio',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_dmd_resumed:'DMD reanudado',msg_load_error:'No se pudo cargar la configuración',save:'&#x1F4BE; Guardar',resume:'&#x25B6; Reanudar DMD',saved:'Guardado',save_err:'Error al guardar',resumed:'DMD reanudado',loading:'Cargando...'}
};

/* cfg_i18n_extra.js -- traductions communes ajoutees a la main (le lien « Reglages » du menu de navigation, v347). Charge APRES cfg_i18n.js. */
if (window.CFG_I18N) {
  CFG_I18N.fr.nav_settings = '&#x2699; Réglages';
  CFG_I18N.en.nav_settings = '&#x2699; Settings';
  CFG_I18N.es.nav_settings = '&#x2699; Ajustes';
  /* v357 : planificateur de luminosite (page Reglages : bouton + case) */
  CFG_I18N.fr.bs_label = 'Planificateur de luminosité'; CFG_I18N.fr.bs_open = 'Configurer';
  CFG_I18N.en.bs_label = 'Brightness scheduler'; CFG_I18N.en.bs_open = 'Configure';
  CFG_I18N.es.bs_label = 'Programador de brillo'; CFG_I18N.es.bs_open = 'Configurar';
}

