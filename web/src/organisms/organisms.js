// Organismes : les panneaux de la feuille de la base, assemblés à partir des molécules.
// Chaque panneau reçoit un « modèle de vue » préparé par le jeu (game.js) : aucun accès direct à l'état du jeu ici.
var UI = window.UI || (window.UI = {});

UI.needBase = () =>
  UI.note('Rejoins la base flottante (par la colonne lumineuse, ou le bras accroché dessous) pour l’utiliser. Talents et options restent accessibles d’ici.');

UI.panels = {
  // v : {atBase, rows (HTML des lignes de minerai), total (texte), canSell}
  sell: v => !v.atBase ? UI.needBase() :
    UI.sum({label: 'À vendre', value: `${v.total} ${UI.coin()}`}) +
    (v.rows.join('') || UI.note('Rien à vendre. Descends chercher du minerai.')) +
    UI.button({label: 'Tout vendre', action: 'sell', variant: 'big', disabled: !v.canSell}) +
    UI.note('Le minerai fondu en lingots vaut 2,5 fois plus. Passe par la fonderie avant de vendre si tu as le temps.'),

  // v : {title, desc, progress (% ou null), atBase, raw, perText, bars (HTML)}
  furn: v =>
    UI.row({title: v.title, desc: v.desc}) +
    (v.progress != null ? UI.meter(v.progress) : '') +
    (v.atBase ? UI.button({label: `Fondre la soute (${v.raw})`, action: 'smelt', variant: 'big', disabled: !v.raw}) : UI.needBase()) +
    UI.note(`${v.perText} s par minerai. Le four continue de tourner quand le jeu est fermé, jusqu’à 8 h. Les lingots se vendent au comptoir.`) +
    (v.bars ? UI.sum({label: 'Lingots prêts'}) + v.bars : ''),

  // v : {coins, atBase, upgrades: [{key, name, level, max, desc, effect, next, isMax, costText, disabled}]}
  shop: v =>
    UI.sum({label: 'Tes pièces', value: `${v.coins} ${UI.coin()}`}) +
    v.upgrades.map(u => UI.row({
      title: u.name, meta: `niv. ${u.level}/${u.max}`, desc: `${u.desc} · ${u.effect}${u.isMax ? '' : ' → ' + u.next}`,
      aside: UI.button({label: u.isMax ? 'Max' : `${u.costText} ${UI.coin()}`, action: 'up', key: u.key, disabled: u.disabled}),
    })).join('') +
    (v.atBase ? '' : UI.needBase()),

  // v : {lvl, xp, need, points, talents: [{key, name, level, desc, disabled}]}
  tal: v =>
    UI.sum({label: `Niveau ${v.lvl} · ${v.xp}/${v.need} XP`, value: `${v.points} point${v.points > 1 ? 's' : ''}`}) +
    v.talents.map(t => UI.row({title: t.name, meta: `${t.level}/5`, desc: t.desc, aside: UI.button({label: '+1', action: 'tal', key: t.key, disabled: t.disabled})})).join('') +
    UI.note('Chaque niveau donne 1 point. L’XP vient du forage, des créatures et des gardiens.'),

  // v : {music, sfx, vib, stick, record, bosses, playTime, confirmWorld, confirmReset}
  opts: v =>
    UI.option({id: 'o-music', label: 'Musique', control: UI.range({id: 'o-music', value: v.music})}) +
    UI.option({id: 'o-sfx', label: 'Effets', control: UI.range({id: 'o-sfx', value: v.sfx})}) +
    UI.option({id: 'o-vib', label: 'Vibrations', control: UI.checkbox({id: 'o-vib', checked: v.vib})}) +
    UI.option({id: 'o-stick', label: 'Joystick', control: UI.select({id: 'o-stick', value: v.stick, options: [['dyn', 'Là où je touche'], ['fixed', 'Fixe, en bas à gauche']]})}) +
    UI.stats(`Record : ${v.record} m · Gardiens vaincus : ${v.bosses}/4<br>Temps de jeu : ${v.playTime}`) +
    UI.note('Aucune pub, aucun traqueur, aucune connexion : la partie est enregistrée sur cet appareil seulement.') +
    (v.confirmWorld
      ? UI.confirmBox({tone: 'confirm', text: 'Créer un nouveau sous-sol ? Tu gardes tes pièces, améliorations, talents, niveau, soute et lingots. Le terrain creusé est perdu et les gardiens reviennent.',
          buttons: [{label: 'Oui, nouveau monde', action: 'world2'}, {label: 'Annuler', action: 'world0', variant: 'warn'}]})
      : UI.button({label: 'Nouveau monde (garde ta progression)', action: 'world1'})) +
    (v.confirmReset
      ? UI.confirmBox({tone: 'danger', text: 'Effacer la partie ? Toute la progression sera perdue.',
          buttons: [{label: 'Oui, tout effacer', action: 'reset2', variant: 'warn'}, {label: 'Annuler', action: 'reset0'}]})
      : UI.button({label: 'Recommencer à zéro', action: 'reset1', variant: 'warn'})),
};
