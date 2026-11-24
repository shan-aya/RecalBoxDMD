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
    sc: { fr: 'Mise à jour des scripts Recalbox disponible : lot {v} (lot {c} installé{r}) — télécharger le toolkit puis lancer le Mode 9', en: 'Recalbox scripts update available: bundle {v} (bundle {c} installed{r}) — download the toolkit, then run Mode 9', es: 'Actualización de los scripts de Recalbox disponible: lote {v} (lote {c} instalado{r}) — descargar el toolkit y ejecutar el Modo 9' },
    tk: { fr: 'Dernier toolkit PC publié : build {v} — à télécharger si le vôtre est plus ancien', en: 'Latest PC toolkit published: build {v} — download it if yours is older', es: 'Última caja de herramientas del PC publicada: build {v} — descárguela si la suya es anterior' }   /* le DMD ne connait pas la version du toolkit installe sur le PC : avis une fois par build publie, a fermer */
  };
  /* v353 : lien vers les notes de version (CHANGELOG dans la langue de la page) a cote de chaque avis de mise a jour ; l'adresse s'affiche aussi au survol */
  var CL = { fr: ['Notes de version', '.fr'], en: ['Release notes', ''], es: ['Notas de versión', '.es'] };
  function clLang() { var l = (localStorage.getItem('dmd_lang') || document.documentElement.lang || 'fr').substring(0, 2); return CL[l] ? l : 'fr'; }
  /* v428 : CANAL DU BUILD (stable | alpha, verrouille a la compilation) : adresses recues dans /load (fw_ru = releases, fw_channel -> branche du CHANGELOG) ; stable par defaut */
  var UR = { ru: 'https://github.com/shan-aya/RecalBoxDMD/releases/latest', br: 'main' };
  function clUrl() { return 'https://github.com/shan-aya/RecalBoxDMD/blob/' + UR.br + '/CHANGELOG' + CL[clLang()][1] + '.md'; }
  var DL = { fr: 'Télécharger le toolkit', en: 'Download the toolkit', es: 'Descargar el toolkit' };   /* v387 : lien vers la derniere version du toolkit PC, a cote des notes de version (avis scripts / toolkit) */
  /* v430 : NOTES DE VERSION PRECISES. cfgNotes(version) | cfgNotes('', lot_de_scripts, version_toolkit_de_repli) : lien vers l'ENTREE du CHANGELOG du composant (ancre GitHub) et bulle de survol limitee a cette entree ;
     sans argument : CHANGELOG entier (ancien comportement). Le CHANGELOG est lu une fois par langue et par page ; hors ligne : le lien reste celui du CHANGELOG entier. */
  var clMd = {};
  function clGet(lg, cb) {
    if (clMd[lg] !== undefined) { cb(clMd[lg]); return; }
    fetch('https://raw.githubusercontent.com/shan-aya/RecalBoxDMD/' + UR.br + '/CHANGELOG' + CL[lg][1] + '.md').then(function (r) { return r.text(); }).then(function (m) { clMd[lg] = m; cb(m); }).catch(function () { cb(''); });
  }
  function clSlug(h) { var s = h.toLowerCase().replace(/[`*_\[\]]/g, ''); try { s = s.replace(/[^\p{L}\p{N}\s_-]/gu, ''); } catch (e) { s = s.replace(/[^\w\s-]/g, ''); } return s.replace(/\s/g, '-'); }
  function clClean(p) { return p.replace(/\[([^\]]*)\]\([^)]*\)/g, '$1').replace(/\*\*|`|\*/g, '').replace(/^- /gm, '• ').replace(/\n{3,}/g, '\n\n').trim(); }
  function clFind(md, nt, ns, nf) {
    var parts = md.replace(/\r/g, '').split(/^## /m).slice(1), i, re, pick = null;
    function byTok(t) { var r = new RegExp('(^|[^\\d.])' + t.replace(/\./g, '\\.') + '(?![\\d])'); for (var j = 0; j < parts.length; j++) if (r.test(parts[j].split('\n')[0])) return parts[j]; return null; }
    if (nt) pick = byTok(nt);
    else if (ns) {   /* « lot de scripts n°9 », « scripts bundle 9 », « scripts Recalbox version 10 » ; jamais « script v11 » (version d'UN script) ; pas d'entree propre : repli sur celle du toolkit qui embarque le lot */
      re = new RegExp('(?:bundle|lots?|lotes?|ensemble|conjunto)[^\\n\\d]{0,40}?' + ns + '(?![\\d.])|(?:scripts Recalbox|Recalbox scripts)\\s+(?:version|versi.n|n°|no\\.|n\\.º|nº)\\s*' + ns + '(?![\\d.])', 'i');
      for (i = 0; i < parts.length && !pick; i++) if (re.test(parts[i])) pick = parts[i];
      if (pick && !re.test(pick.split('\n')[0])) {   /* v431 : le lot n'est cite que DANS une ligne de l'entree (titre = firmware / demon / toolkit) : seules ces lignes comptent */
        var hd = pick.split('\n')[0], bl = pick.substring(hd.length).split(/\n(?=- )/), sel = [];
        for (var k = 0; k < bl.length; k++) if (re.test(bl[k])) sel.push(bl[k]);
        var tt = clClean(hd.split(' \u2014 ')[0] + '\n\n' + sel.join('\n'));
        return { head: hd, text: tt.length > 1500 ? tt.substring(0, 1500).replace(/\s+\S*$/, '') + ' \u2026' : tt };
      }
    }
    if (!pick) return null;
    var t = clClean(pick);
    return { head: pick.split('\n')[0], text: t.length > 1500 ? t.substring(0, 1500).replace(/\s+\S*$/, '') + ' …' : t };
  }
  function clAnch(el, lg, e) { el.href = 'https://github.com/shan-aya/RecalBoxDMD/blob/' + UR.br + '/CHANGELOG' + CL[lg][1] + '.md#' + clSlug(e.head); }
  function clResolve(el) {
    var nt = el.getAttribute('data-nt') || '', ns = el.getAttribute('data-ns') || '', nf = el.getAttribute('data-nf') || '';
    if (!nt && !ns) return;
    var lg = clLang();
    clGet(lg, function (md) { var e = md ? clFind(md, nt, ns, nf) : null; if (e) clAnch(el, lg, e); });
  }
  window.cfgNotes = function (nt, ns, nf) {
    nt = String(nt || '').replace(/[^\d.]/g, ''); ns = String(ns || '').replace(/[^\d]/g, ''); nf = String(nf || '').replace(/[^\d.]/g, '');
    var a = (nt ? ' data-nt="' + nt + '"' : '') + (ns ? ' data-ns="' + ns + '"' + (nf ? ' data-nf="' + nf + '"' : '') : '');
    if (a) setTimeout(function () { var l = document.querySelectorAll('a.cfgcl[data-nt],a.cfgcl[data-ns]'); for (var i = 0; i < l.length; i++) clResolve(l[i]); }, 300);
    return '<a class="cfgcl" href="' + clUrl() + '"' + a + ' target="_blank" rel="noopener">' + CL[clLang()][0] + '</a>';
  };
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
    var nt = el.getAttribute('data-nt') || '', ns = el.getAttribute('data-ns') || '';
    if (nt || ns) { put('\u2026'); clGet(lg, function (md) { var e = md ? clFind(md, nt, ns, el.getAttribute('data-nf') || '') : null; if (e) { clAnch(el, lg, e); if (clTip && clTip.style.display === 'block') put(e.text); } else if (clTip) clTip.style.display = 'none'; }); return; }   /* v430 : entree du composant seule */
    if (clTxt[lg]) { put(clTxt[lg]); return; }
    put('…');
    fetch('https://raw.githubusercontent.com/shan-aya/RecalBoxDMD/' + UR.br + '/CHANGELOG' + CL[lg][1] + '.md').then(function (r) { return r.text(); }).then(function (m) { clTxt[lg] = clParse(m); if (clTip && clTip.style.display === 'block') put(clTxt[lg]); }).catch(function () { if (clTip) clTip.style.display = 'none'; });
  }
  document.addEventListener('mouseover', function (e) { var t = e.target && e.target.closest ? e.target.closest('.cfgcl') : null; if (t) clShow(t); });
  document.addEventListener('mouseout', function (e) { var t = e.target && e.target.closest ? e.target.closest('.cfgcl') : null; if (t && clTip) clTip.style.display = 'none'; });
  /* v428 : lien de telechargement du toolkit = la RELEASE EXACTE annoncee par le manifeste du canal (tag RecalBoxDMD_tool_v<build> : release stable ou pre-release alpha) ; sans numero : page des releases du canal (UR.ru) */
  function tkUrl(n) { n = parseInt(n, 10) || 0; return n > 0 ? 'https://github.com/shan-aya/RecalBoxDMD/releases/tag/RecalBoxDMD_tool_v' + n : UR.ru; }
  window.cfgTk = function (n) { return tkUrl(n); };   /* v430 : lien de la release EXACTE du toolkit pour les pages (Reglages) */
  function rbTxt(ip) { return ip ? ' \u00b7 Recalbox ' + ip : ''; }   /* v431 : l'avis des scripts nomme la Recalbox concernee (celle qui alimente ce DMD : recalbox_active) */
  function updBar(d, latest, scrLatest, tkLatest) {
    var onSet = location.pathname === '/config/settings';   /* la page Reglages a ses propres avis firmware / scripts : seul l'avis toolkit s'y ajoute */
    var lg = (localStorage.getItem('dmd_lang') || document.documentElement.lang || 'fr').substring(0, 2); if (!TXT.fw[lg]) lg = 'fr';
    var cur = String((d && d.fw_version) || ''), items = [];
    var fwNew = vnum(cur) > 0 && vnum(latest) > vnum(cur);
    if (!onSet && fwNew) items.push({ k: 'fw', v: latest, c: cur, t: TXT.fw[lg].replace('{v}', latest).replace('{c}', cur), h: '/config/settings', b: false, nt: latest });
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
    if (!onSet && sl > 0 && have >= 0 && have < sl) items.push({ k: 'sc', v: sl, c: have, r: String((d && d.recalbox_active) || ''), t: TXT.sc[lg].replace('{v}', sl).replace('{c}', have).replace('{r}', rbTxt(d && d.recalbox_active)), h: tkUrl(tkLatest), b: true, ns: String(sl), nf: String(parseInt(tkLatest, 10) || '') });
    var tkn = parseInt(tkLatest, 10) || 0, tks = 0; try { tks = parseInt(localStorage.getItem(TKS) || '0', 10) || 0; } catch (e) {}
    if (tkn > 0 && tkn > tks) items.push({ k: 'tk', v: tkn, t: TXT.tk[lg].replace('{v}', tkn), h: tkUrl(tkn), b: true, tk: tkn, nt: String(tkn) });
    if (!items.length) return;
    var show = function () {
      if (document.getElementById('cfgFwBar') || !document.body) return;
      var w = document.createElement('div'); w.id = 'cfgFwBar'; w.style.cssText = 'margin:46px 0 10px';   /* v359 : sous le selecteur de langue / le lien Aide (positionnes en haut a droite), qui masquaient la fin du bandeau */
      items.forEach(function (it) {
        var r = document.createElement('div'), a = document.createElement('a'), n = document.createElement('a');
        a.href = it.h; if (it.b) { a.target = '_blank'; a.rel = 'noopener'; } a.textContent = it.t; a.style.cssText = 'color:inherit;text-decoration:none';
        n.href = clUrl(); n.target = '_blank'; n.rel = 'noopener'; n.className = 'cfgcl'; n.textContent = CL[clLang()][0]; if (it.nt) n.setAttribute('data-nt', it.nt); if (it.ns) { n.setAttribute('data-ns', it.ns); if (it.nf) n.setAttribute('data-nf', it.nf); } clResolve(n); n.style.cssText = 'color:inherit;text-decoration:underline';
        var nr = document.createElement('div'); nr.style.cssText = 'margin-top:3px'; nr.appendChild(n);
        if (it.b) { var dl = document.createElement('a'); dl.href = it.h; dl.target = '_blank'; dl.rel = 'noopener'; dl.style.cssText = 'color:inherit;text-decoration:underline;margin-left:16px'; dl.textContent = DL[clLang()]; it.d = dl; nr.appendChild(dl); }
        r.style.cssText = 'text-align:center;background:#7a4a12;color:#ffcf8a;font:700 13px "Segoe UI",Tahoma,sans-serif;padding:8px 10px;margin:0 0 4px;border-radius:8px';
        it.a = a; it.n = n;   /* v384 : references gardees pour retraduire le bandeau quand la langue de la page change */
        r.appendChild(a); r.appendChild(nr); w.appendChild(r);
        if (it.tk) { var x = document.createElement('span'); x.textContent = '\u2715'; x.title = 'OK'; x.style.cssText = 'float:left;cursor:pointer;padding:0 4px;margin:-2px 6px 0 0'; x.onclick = function () { try { localStorage.setItem(TKS, String(it.tk)); } catch (e) {} r.parentNode && r.parentNode.removeChild(r); }; r.insertBefore(x, r.firstChild); }
      });
      document.body.insertBefore(w, document.body.firstChild);
      /* v384 : le bandeau etait construit UNE fois dans la langue du chargement (souvent le francais avant que la page applique la langue du DMD) et ne suivait pas le selecteur de langue : il est retraduit a chaque changement de l'attribut lang de la page */
      var retr = function () {
        var l2 = 'fr'; try { l2 = (localStorage.getItem('dmd_lang') || document.documentElement.lang || 'fr').substring(0, 2); } catch (e) { l2 = (document.documentElement.lang || 'fr').substring(0, 2); } if (!TXT.fw[l2]) l2 = 'fr';
        items.forEach(function (it) { if (!it.a) return; it.a.textContent = TXT[it.k][l2].replace('{v}', it.v).replace('{c}', it.c === undefined ? '' : it.c).replace('{r}', rbTxt(it.r)); it.n.textContent = CL[clLang()][0]; it.n.href = clUrl(); clResolve(it.n); if (it.d) it.d.textContent = DL[clLang()]; });
      };
      retr();
      try { new MutationObserver(retr).observe(document.documentElement, { attributes: true, attributeFilter: ['lang'] }); } catch (e) {}
    };
    if (document.body) show(); else document.addEventListener('DOMContentLoaded', show);
  }
  /* v429b : MISE EN PAGE DE LA PAGE REGLAGES (demande utilisateur : memes cadres et meme charte que les autres sections). Construite ICI apres le chargement de /load, a partir de la page
     existante (la page elle-meme est au plafond de poids gzip) :
       - section « Affichage » : luminosite + planificateur + ORDRE DES COULEURS de la dalle (reglages de l'ecran, regroupes) ;
       - nouvelle section « Demarrage et modes » : demarrage silencieux, mode Pinball (VPX), alertes RetroAchievements (lignes DEPLACEES, memes ids : la sauvegarde de la page ne change pas) ;
       - nouvelle section « Canal du firmware » sous le bloc de version : canal du build + CHANGEMENT DE CANAL stable <-> alpha (POST /ota/start?chan=, mise a jour forcee depuis un jeu d'adresses FIXE).
     Ordre des couleurs : six ordres RGB / RBG / GRB / GBR / BRG / BGR ; « GRB » : l'entree R de la dalle recoit le canal G (meme sens que « led-rgb-sequence » de hzeller) ; le DMD enregistre (NVS + config.ini) puis redemarre. */
  var SLT = {
    fr: { chanH: '🔄 Canal du firmware', chanCur: 'Canal actuel', chanSw: 'Passer au canal ', chanHint: 'Télécharge et installe le firmware de l’autre canal, puis redémarre (la version du canal choisi peut être plus ancienne que celle installée).',
          chanC: 'Le DMD va télécharger et installer le firmware du canal {c}{v}, puis redémarrer (environ 1 minute). La version du canal choisi peut être plus ancienne que celle installée. Continuer ?', chanRun: 'Mise à jour en cours… ne débranchez pas le DMD.', chanOk: 'Terminé : firmware v{v} installé.', chanTo: 'Délai dépassé : vérifiez le DMD.',
          bootH: '🚀 Démarrage et modes', rgbL: 'Ordre des couleurs', rgbB: 'Appliquer et redémarrer', rgbH: 'Si les couleurs sont inversées (par exemple le rouge s’affiche en bleu ou en vert), essayez un autre ordre : choisir un ordre affiche R, G, B en gros sur le DMD pendant 30 s (chaque lettre doit avoir sa couleur) ; le DMD ne redémarre qu’en appuyant sur Appliquer.',
          rgbC: 'Appliquer l’ordre {o} ? Le DMD va redémarrer.', rgbRun: 'Redémarrage du DMD…', rgbOk: 'Ordre {o} appliqué.', ko: 'Échec : {e}' },
    en: { chanH: '🔄 Firmware channel', chanCur: 'Current channel', chanSw: 'Switch to channel ', chanHint: 'Downloads and installs the firmware of the other channel, then restarts (the chosen channel may be older than the installed version).',
          chanC: 'The DMD will download and install the {c} channel firmware{v}, then restart (about 1 minute). The chosen channel may be older than the installed version. Continue?', chanRun: 'Updating… do not unplug the DMD.', chanOk: 'Done: firmware v{v} installed.', chanTo: 'Timed out: check the DMD.',
          bootH: '🚀 Startup and modes', rgbL: 'Colour order', rgbB: 'Apply and restart', rgbH: 'If the colours are swapped (for example red shows as blue or green), try another order: choosing one shows R, G, B in large letters on the DMD for 30 s (each letter must have its own colour); the DMD only restarts when you press Apply.',
          rgbC: 'Apply order {o}? The DMD will restart.', rgbRun: 'Restarting the DMD…', rgbOk: 'Order {o} applied.', ko: 'Failed: {e}' },
    es: { chanH: '🔄 Canal del firmware', chanCur: 'Canal actual', chanSw: 'Cambiar al canal ', chanHint: 'Descarga e instala el firmware del otro canal y reinicia (la versión del canal elegido puede ser más antigua que la instalada).',
          chanC: 'El DMD descargará e instalará el firmware del canal {c}{v} y se reiniciará (aprox. 1 minuto). La versión del canal elegido puede ser más antigua que la instalada. ¿Continuar?', chanRun: 'Actualizando… no desconecte el DMD.', chanOk: 'Listo: firmware v{v} instalado.', chanTo: 'Tiempo agotado: compruebe el DMD.',
          bootH: '🚀 Arranque y modos', rgbL: 'Orden de colores', rgbB: 'Aplicar y reiniciar', rgbH: 'Si los colores están invertidos (por ejemplo el rojo se ve azul o verde), pruebe otro orden: al elegir uno, el DMD muestra R, G, B en letras grandes durante 30 s (cada letra debe tener su color); el DMD solo se reinicia al pulsar Aplicar.',
          rgbC: '¿Aplicar el orden {o}? El DMD se reiniciará.', rgbRun: 'Reiniciando el DMD…', rgbOk: 'Orden {o} aplicado.', ko: 'Error: {e}' }
  };
  var M_STABLE = 'https://shan-aya.github.io/RecalBoxDMD/install/manifest.json', M_ALPHA = 'https://raw.githubusercontent.com/shan-aya/RecalBoxDMD/alpha/docs/install/manifest.json';
  function settingsLayout(d) {
    if (document.getElementById('rbChan') || !d.fw_channel) return;
    var br = document.getElementById('brightness'), fwbox = document.querySelector('.fwbox');
    if (!br || !fwbox) return;                                                 /* page Reglages seulement, une seule fois */
    var disp = br.closest('.section'); if (!disp) return;
    var refs = [], isPi = d.platform === 'rpi';
    function T(k) { return (SLT[clLang()] || SLT.fr)[k]; }
    function txt(el, k, suffix) { refs.push([el, k, suffix || '']); el.textContent = T(k) + (suffix || ''); return el; }
    function mk(tag, cls, st) { var e = document.createElement(tag); if (cls) e.className = cls; if (st) e.style.cssText = st; return e; }
    var HB = 'background:#0f3460;color:#eee;margin-left:14px';                 /* meme style que le bouton « Configurer » du planificateur */
    /* 1) section « Demarrage et modes » : lignes deplacees (memes ids) */
    var moved = ['silent_boot', 'feat_vpinball_dmd', 'feat_ra_window'].map(function (id) { var e = document.getElementById(id); return e && e.closest('.row'); }).filter(Boolean);
    if (moved.length) {
      var sb = mk('div', 'section'); sb.id = 'rbBoot'; sb.appendChild(txt(mk('h2'), 'bootH'));
      moved.forEach(function (r) { sb.appendChild(r); });
      disp.parentNode.insertBefore(sb, disp.nextSibling);
    }
    /* 2) ordre des couleurs : ligne de la section « Affichage », juste apres le planificateur */
    if (d.rgb_order) {
      var cur = String(d.rgb_order), first = br.closest('.row') || disp.querySelector('.row');                /* ordre des couleurs AU-DESSUS de la luminosite */
      var row = mk('div', 'row'); row.id = 'rbRgb';
      var sel = mk('select'); ['RGB', 'RBG', 'GRB', 'GBR', 'BRG', 'BGR'].forEach(function (o) { var op = mk('option'); op.value = o; op.textContent = o; if (o === cur) op.selected = true; sel.appendChild(op); });
      var btn = mk('button', 'hbtn', HB); btn.type = 'button';
      row.appendChild(txt(mk('label'), 'rgbL')); row.appendChild(sel); row.appendChild(txt(btn, 'rgbB'));
      var hint = txt(mk('div', 'hint'), 'rgbH'), st = mk('div', 'hint', 'color:#ffd146');
      first.parentNode.insertBefore(row, first); first.parentNode.insertBefore(hint, first); first.parentNode.insertBefore(st, first);
      /* v3.89 : APERCU sur le DMD : choisir un ordre affiche R, G, B en gros (chaque lettre dans sa couleur, 30 s) ; la fermeture de la page rend l'ecran */
      sel.onchange = function () { window.fetch('/clock-preview', { method: 'POST', body: 'theme=c' + sel.value, headers: { 'Content-Type': 'application/x-www-form-urlencoded' } }).catch(function () {}); };
      window.addEventListener('pagehide', function () { try { navigator.sendBeacon('/clock-preview', new Blob(['theme=stop'], { type: 'application/x-www-form-urlencoded' })); } catch (e) {} });
      btn.onclick = function () {
        var v = sel.value; if (v === cur) return;
        if (!window.confirm(T('rgbC').replace('{o}', v))) return;
        btn.disabled = true; sel.disabled = true; st.textContent = T('rgbRun');
        window.fetch('/rgb-order?v=' + v, { method: 'POST' }).catch(function () {});
        var t0 = Date.now();
        (function poll() {
          setTimeout(function () {
            if (Date.now() - t0 > 90000) { st.textContent = T('ko').replace('{e}', 'timeout'); btn.disabled = false; sel.disabled = false; return; }
            of('/load?_=' + Date.now(), { cache: 'no-store' }).then(function (r) { return r.json(); }).then(function (x) {
              if (String(x.rgb_order) === v) { st.textContent = T('rgbOk').replace('{o}', v); setTimeout(function () { location.reload(); }, 2500); } else poll();
            }).catch(poll);
          }, 3000);
        })();
      };
    }
    /* 3) section « Canal du firmware » sous le bloc de version */
    var ch = d.fw_channel === 'alpha' ? 'alpha' : 'stable', other = ch === 'alpha' ? 'stable' : 'alpha';
    var cs = mk('div', 'section'); cs.id = 'rbChan'; cs.appendChild(txt(mk('h2'), 'chanH'));
    var r1 = mk('div', 'row'); r1.appendChild(txt(mk('label'), 'chanCur'));
    var cv = mk('b', null, 'color:#ffd146'); cv.textContent = ch.toUpperCase() + (d.fw_version ? ' (v' + d.fw_version + ')' : ''); r1.appendChild(cv);
    var cb = mk('button', 'hbtn', HB); cb.type = 'button';   /* bouton sur la MEME ligne que le canal actuel */
    r1.appendChild(cb); refs.push([cb, 'chanSw', other.toUpperCase() + '…']); cb.textContent = T('chanSw') + other.toUpperCase() + '…';
    var ch1 = txt(mk('div', 'hint'), 'chanHint'), cst = mk('div', 'hint', 'color:#ffd146');
    cs.appendChild(r1); cs.appendChild(ch1); cs.appendChild(cst);
    fwbox.parentNode.insertBefore(cs, fwbox.nextSibling);
    /* boutons « Passer au canal », « Appliquer et redemarrer » et « Configurer » : meme taille, alignes sur la meme colonne */
    function alignBtns() {
      var cf = document.querySelector('#bright_sched_on ~ a.hbtn'); if (!cf || !cf.offsetParent || !sel || !sel.parentNode) return;
      var W = 190, all = [cb, btn, cf];
      all.forEach(function (b) { b.style.cssText += ';width:' + W + 'px;box-sizing:border-box;text-align:center;padding:9px 0;line-height:1.2;flex:0 0 ' + W + 'px;margin-left:0'; });
      sel.style.width = '130px'; sel.style.flex = '0 0 130px'; cv.style.cssText += ';display:inline-block;width:130px;flex:0 0 130px';
      var rr = sel.parentNode.getBoundingClientRect(), sl = sel.getBoundingClientRect().left;
      var bl = ((sl + 130) + (rr.right - 8)) / 2 - W / 2;                              /* bouton « Appliquer » CENTRE entre le selecteur RGB et le bord droit du cadre */
      
      btn.style.marginLeft = Math.max(0, bl - sl - 130) + 'px'; cb.style.marginLeft = btn.style.marginLeft;
      var cl = cf.getBoundingClientRect().left; cf.style.marginLeft = (bl - cl) + 'px';
      var cl2 = cf.getBoundingClientRect().left; if (Math.abs(cl2 - bl) > 1) cf.style.marginLeft = (bl - cl + (bl - cl2)) + 'px';   /* 2e passe : corrige l'ecart residuel */   /* « Configurer » aligne sur le meme x */
    }
    alignBtns(); window.addEventListener('resize', alignBtns); setTimeout(alignBtns, 400);
    function runSwitch() {
      cb.disabled = true; cst.textContent = T('chanRun');
      window.fetch('/ota/start?lang=' + clLang() + '&chan=' + other, { method: 'POST' }).catch(function () {});
      var t0 = Date.now();
      (function poll() {
        setTimeout(function () {
          if (Date.now() - t0 > 360000) { cst.textContent = T('chanTo'); cb.disabled = false; return; }
          of('/load?_=' + Date.now(), { cache: 'no-store' }).then(function (r) { return r.json(); }).then(function (x) {
            var o = String(x.ota_last || '');
            if (o === '' || o === 'en-cours') { poll(); return; }
            if (o.indexOf('ok:') === 0) { cst.textContent = T('chanOk').replace('{v}', o.replace(/^ok:(deja-a-jour\()?/, '').replace(/\)$/, '')); setTimeout(function () { location.reload(); }, 4000); return; }
            cst.textContent = T('ko').replace('{e}', o.replace(/^echec:/, '')); cb.disabled = false;
          }).catch(poll);
        }, 3000);
      })();
    }
    cb.onclick = function () {
      var ask = function (v) { if (window.confirm(T('chanC').replace('{c}', other.toUpperCase()).replace('{v}', v ? ' (v' + v + ')' : ''))) runSwitch(); };
      var c = new AbortController(), to = setTimeout(function () { c.abort(); }, 6000);
      of(other === 'alpha' ? M_ALPHA : M_STABLE, { cache: 'no-store', signal: c.signal }).then(function (r) { return r.json(); }).then(function (m) {
        clearTimeout(to); ask(String((m && (isPi && m.pi ? m.pi : m.version)) || ''));
      }).catch(function () { clearTimeout(to); ask(''); });
    };
    /* langue : les textes ajoutes suivent le selecteur de langue de la page */
    try { new MutationObserver(function () { refs.forEach(function (r) { r[0].textContent = T(r[1]) + r[2]; }); }).observe(document.documentElement, { attributes: true, attributeFilter: ['lang'] }); } catch (e) {}
  }
  function fwCheck(d) {
    if (!d || !d.fw_version) return;
    try { settingsLayout(d); } catch (e) {}
    if (d.fw_ru) UR.ru = d.fw_ru;
    UR.br = d.fw_channel === 'alpha' ? 'alpha' : 'main';
    if (UR.br === 'alpha' && !document.getElementById('rbAlpha')) {   /* ruban ALPHA sur toutes les pages : build de test identifiable d'un coup d'oeil (captures du support) */
      var ab = document.createElement('div'); ab.id = 'rbAlpha'; ab.textContent = 'ALPHA';
      ab.style.cssText = 'position:fixed;top:0;right:0;z-index:9999;background:#b91c1c;color:#fff;font:bold 12px sans-serif;padding:3px 10px;border-radius:0 0 0 6px';
      document.body.appendChild(ab);
    }
    /* v3.97 : un Pi se compare a la version du demon Pi (cle pi), un ESP32 a celle du firmware (cle version) ; window._cfgPi est aussi lu par la page Reglages */
    var isPi = window._cfgPi = d.platform === 'rpi' || (!d.platform && d.boot_reason === 'raspberry-pi');
    var now = Date.now(), t = 0, latest = '', scr = '', tk = '';
    try { t = parseInt(localStorage.getItem(FWT) || '0', 10) || 0; latest = localStorage.getItem(FWK) || ''; scr = localStorage.getItem(SCK) || ''; tk = localStorage.getItem(TKK) || ''; } catch (e) {}
    /* v429b : le manifeste mis en cache est CELUI D'UN CANAL : apres un changement de canal (stable <-> alpha) on ne reutilise jamais l'annonce de l'autre canal (elle restait affichee jusqu'a 5 min) */
    try { if ((localStorage.getItem('dmd_fw_manifest_c') || '') !== String(d.fw_channel || '')) { latest = ''; scr = ''; tk = ''; } } catch (e) {}
    if (latest && now - t < 5 * 60 * 1000) { updBar(d, latest, scr, tk); return; }
    var c = new AbortController(), to = setTimeout(function () { c.abort(); }, 6000);
    of(d.fw_mu || 'https://shan-aya.github.io/RecalBoxDMD/install/manifest.json', { cache: 'no-store', signal: c.signal }).then(function (r) { return r.json(); }).then(function (m) {
      latest =String((m && (isPi && m.pi ? m.pi : m.version)) || ''); scr = String((m && m.scripts) || ''); tk = String((m && m.toolkit) || '');
      try { localStorage.setItem(FWK, latest); localStorage.setItem(SCK, scr); localStorage.setItem(TKK, tk); localStorage.setItem(FWT, String(now)); localStorage.setItem('dmd_fw_manifest_c', String(d.fw_channel || '')); } catch (e) {}
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

/* v388 : page de configuration fermee (croix, fermeture du navigateur, plantage) -> le DMD ne reste plus en mode config.
   - pagehide : « je pars » (POST /dmd-leave par sendBeacon) : le DMD reprend 10 s plus tard, sauf si une page se rouvre dans ce delai (F5, autre page de configuration) ;
   - tant que l'onglet est visible : battement POST /dmd-hb toutes les 20 s ; sans battement ni message pendant 3 min (navigateur plante, Wi-Fi coupe, onglet masque) le DMD reprend seul ;
   - retour sur un onglet masque : le DMD est remis en mode config (il a pu reprendre entre-temps). */
(function () {
  var hbT = null;
  function hb() { if (!document.hidden) fetch('/dmd-hb', { method: 'POST' }).catch(function () {}); }
  function start() { if (!hbT) hbT = setInterval(hb, 20000); }
  function stop() { if (hbT) { clearInterval(hbT); hbT = null; } }
  function rearm() { fetch('/dmd-pause', { method: 'POST', body: new URLSearchParams({ msg: 'http://' + location.host, color: '' }), headers: { 'Content-Type': 'application/x-www-form-urlencoded' } }).catch(function () {}); }
  document.addEventListener('visibilitychange', function () { if (document.hidden) stop(); else { rearm(); start(); } });
  window.addEventListener('pagehide', function () { try { navigator.sendBeacon('/dmd-leave', new Blob([''], { type: 'text/plain' })); } catch (e) {} });
  window.addEventListener('pageshow', function (e) { if (e.persisted) { rearm(); start(); } });
  start();
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

/* v359 -- mention d'auteur + lien « Payez-moi un cafe » en bas de TOUTES les pages (sous l'image d'intro sur l'accueil, qui a .logo-wrap). Langue = dmd_lang, mise a jour au changement de langue. */
(function () {
  var TXT = { fr: 'Payez-moi un caf\u00e9', en: 'Buy me a coffee', es: 'Inv\u00edtame a un caf\u00e9' };
  function lang() { var l = ''; try { l = (localStorage.getItem('dmd_lang') || '').substring(0, 2); } catch (e) {} if (!TXT[l]) l = (document.documentElement.lang || 'fr').substring(0, 2); return TXT[l] ? l : 'fr'; }
  function upd() { var s = document.getElementById('cfgCoffee'); if (s) s.textContent = TXT[lang()]; }
  function build() {
    if (document.getElementById('cfgCredit') || !document.body) return;
    var d = document.createElement('div'); d.id = 'cfgCredit';
    d.style.cssText = 'text-align:center;margin:16px 0 8px;font:12px "Segoe UI",Tahoma,sans-serif;opacity:.85';
    d.innerHTML = '&copy; Shan_ayA 2026 &middot; <a href="https://buymeacoffee.com/shan_aya" target="_blank" rel="noopener" style="color:inherit;text-decoration:underline;text-underline-offset:4px">\u2615 <span id="cfgCoffee">' + TXT[lang()] + '</span></a>';
    var logo = document.querySelector('.logo-wrap');
    if (logo && logo.parentNode) logo.parentNode.insertBefore(d, logo.nextSibling); else document.body.appendChild(d);
  }
  document.addEventListener('change', function (e) { if (e.target && e.target.id === 'langSelect') setTimeout(upd, 0); });
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', build); else build();
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

