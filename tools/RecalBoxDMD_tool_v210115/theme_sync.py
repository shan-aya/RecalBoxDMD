"""Moteur « themes » partage par le Mode 1, le Mode 12 et la page « Themes » : AUCUNE interface, des fonctions pures testables.

Ce module reprend, SANS changer le comportement, la logique qui vivait dans la fenetre du Mode 12 (RecalBoxDMD_theme_logos_gui.py) :
  - mark_package_equivalents : un theme dont le paquet GitHub est EQUIVALENT se met a jour par TELECHARGEMENT (secondes) plutot que par conversion (~30 s) ;
  - is_alpha_default : theme par defaut dont le paquet vient d'une Recalbox alpha (a telecharger, jamais a reconvertir depuis le hub) ;
et ajoute deux lectures simples pour une interface non technique :
  - user_state : l'etat d'un theme en 6 mots clairs (au lieu de NOUVEAU / MAJ / DIFFERENT... + detail en texte libre) ;
  - plan_for : ce que l'on fera pour ce theme (telecharger le paquet, convertir depuis la Recalbox / le hub, theme par defaut).

Une ligne `m` est le dictionnaire de la fenetre du Mode 12 : state, detail, rb (copie installee sur la Recalbox), hub, rb_version, hub_version, sd, pkg.
"""

# Etats lisibles (cles i18n de l'interface)
ON_DMD = "on_dmd"            # le DMD est a jour
TO_SEND = "to_send"          # theme jamais envoye au DMD
UPDATE = "update"            # le DMD a une version plus ancienne que ce que l'on peut lui envoyer
RB_UPDATE = "rb_update"      # le theme installe sur la Recalbox est plus ancien que la derniere version publiee : a mettre a jour d'abord sur la Recalbox
NO_LOGOS = "no_logos"        # aucun logo de systeme exploitable dans ce theme
OTHER_RB = "other_rb"        # present sur le DMD ; installe sur une AUTRE Recalbox du DMD que celle(s) selectionnee(s)
DMD_ONLY = "dmd_only"        # present sur le DMD mais plus installe sur AUCUNE Recalbox : a garder ou a supprimer pour faire de la place
NO_TARGET = "no_target"      # ni carte SD du DMD ni Raspberry Pi cible : l'etat sur le DMD est INCONNU (rien a convertir « pour rien »)
ERROR = "error"

# Plans
PLAN_DEFAULT = "default"     # theme par defaut : paquet GitHub (jamais converti ici)
PLAN_DOWNLOAD = "download"   # paquet GitHub equivalent : telechargement rapide
PLAN_CONVERT = "convert"     # conversion locale (theme perso, patron, version differente, absent du paquet)
PLAN_NONE = "none"           # rien a faire / rien d'exploitable

_RB_OUTDATED_PREFIX = "à mettre à jour sur la Recalbox"   # meme texte que theme_logos.check_status


def merge_rb_rows(per_root, version_newer):
    """Plusieurs Recalbox pour un meme DMD, traitees en UNE operation. per_root = [(racine, lignes)] ou lignes = tuples (dossier, etat, detail, version, source) de theme_logos.check_status.
    Les lignes « hub » ne sont gardees qu'une fois ; un theme present sur plusieurs Recalbox garde la copie la PLUS RECENTE (egalite : la premiere Recalbox de la liste).
    Retourne (lignes, racine_par_theme, conflits) ; conflits = [(dossier, [(racine, version), ...], racine_retenue)] quand les versions different."""
    out, best, seen, hub_seen = [], {}, {}, set()
    for root, rows in per_root:
        for r in rows:
            if r[4] != "rb":
                if (r[0], r[4]) not in hub_seen:
                    hub_seen.add((r[0], r[4]))
                    out.append(r)
                continue
            seen.setdefault(r[0], []).append((root, r[3]))
            cur = best.get(r[0])
            if cur is None or (r[3] is not None and (cur[1][3] is None or version_newer(r[3], cur[1][3]))):
                best[r[0]] = (root, r)
    out.extend(r for _root, r in best.values())
    root_of = {f: root for f, (root, _r) in best.items()}
    conflicts = [(f, lst, root_of[f]) for f, lst in seen.items() if len({str(v) for _root, v in lst}) > 1]
    return out, root_of, conflicts


def package_pipeline(man):
    """Version de pipeline du paquet GitHub (0 si inconnue)."""
    try:
        return int((man or {}).get("pipeline") or 0)
    except (TypeError, ValueError):
        return 0


def mark_package_equivalents(merged, man, tl):
    """v26 (extrait de la fenetre du Mode 12) : un theme a (re)installer dont le paquet GitHub est EQUIVALENT (meme version que la copie de la Recalbox, ou du hub / de Recalbox pour les themes
    sans copie ; rendu du paquet >= rendu du toolkit) se met a jour par TELECHARGEMENT au lieu d'une conversion. m["pkg"] = True. Le reste (version differente, logos modifies sur la
    Recalbox, theme absent du paquet) se convertit comme avant. `tl` = module theme_logos (theme_dir_name, same_version, PIPELINE_VERSION)."""
    pkg_pipe = package_pipeline(man)
    themes = (man or {}).get("themes", {})
    for folder, m in merged.items():
        m["pkg"] = False
        entry = themes.get(tl.theme_dir_name(folder))
        if not entry or pkg_pipe < tl.PIPELINE_VERSION or m["state"] not in ("MAJ", "NOUVEAU"):
            continue
        detail = str(m["detail"])
        if m["state"] == "MAJ" and not (detail.startswith("rendu") or detail.startswith("version inconnue") or detail.startswith("installé depuis le paquet")):
            continue            # logos modifies / langue-region / version plus recente : conversion depuis la Recalbox
        ver = m.get("rb_version") if m.get("rb") else m.get("hub_version")
        if tl.same_version(entry.get("version"), ver):
            m["pkg"] = True
            if m["state"] == "MAJ":
                m["detail"] = "paquet GitHub : " + detail + " (téléchargement)"
            else:
                m["detail"] = "disponible dans le paquet GitHub (téléchargement rapide)"


def is_alpha_default(folder, man, theme_dir_name, default_theme="recalbox-next"):
    """Theme par defaut (recalbox-next) dont le paquet GitHub est issu d'une Recalbox ALPHA (packaged_from « rb ») : plus recent que l'archive publique figee du hub, donc a TELECHARGER
    (SD / Pi), jamais a reconvertir depuis le hub (ancien design)."""
    try:
        if folder != default_theme:
            return False
        ent = ((man or {}).get("themes") or {}).get(theme_dir_name(folder)) or {}
        return ent.get("packaged_from") == "rb"
    except Exception:
        return False


def is_rb_outdated(m):
    """Le theme installe sur la Recalbox est plus ancien que la derniere version publiee (ligne produite par theme_logos.check_status)."""
    return str((m or {}).get("detail", "")).startswith(_RB_OUTDATED_PREFIX)


def user_state(m, target_ok=True):
    """Etat d'un theme pour l'utilisateur : ON_DMD / TO_SEND / UPDATE / RB_UPDATE / NO_LOGOS / ERROR, ou NO_TARGET quand aucune cible (carte SD du DMD / Raspberry Pi) n'est joignable :
    l'etat sur le DMD est alors inconnu (les etats « NOUVEAU / MAJ » ne compareraient qu'avec le dossier de travail du toolkit, vide). « A mettre a jour sur la Recalbox » ne depend pas de la cible."""
    st = (m or {}).get("state")
    if st == "ERREUR":
        return ERROR
    if (m or {}).get("dmd_only"):
        return DMD_ONLY
    if (m or {}).get("other_rb"):
        return OTHER_RB
    if st == "SANS LOGOS":
        return NO_LOGOS
    if is_rb_outdated(m):
        return RB_UPDATE
    if not target_ok:
        return NO_TARGET
    if st == "A JOUR":
        return ON_DMD
    if st == "NOUVEAU":
        return TO_SEND
    return UPDATE                 # MAJ, DIFFERENT, inconnu : le DMD est a mettre a jour


def plan_for(m, default_theme_row=False, target_ok=True):
    """Ce que l'on fera pour mettre ce theme sur le DMD : PLAN_DEFAULT (theme par defaut du paquet), PLAN_DOWNLOAD (paquet equivalent), PLAN_CONVERT (conversion locale)
    ou PLAN_NONE (rien d'exploitable, ou pas de cible). `default_theme_row` : la ligne est le theme par defaut."""
    if user_state(m, target_ok) in (NO_LOGOS, ERROR, NO_TARGET, DMD_ONLY, OTHER_RB):
        return PLAN_NONE
    if default_theme_row:
        return PLAN_DEFAULT
    return PLAN_DOWNLOAD if (m or {}).get("pkg") else PLAN_CONVERT
