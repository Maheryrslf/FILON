// Exporte des données de référence depuis la version JS, pour les tests de parité C++.
// Usage : node tools/export_reference.js docs/reference-js/filon-v17.html tests/data
// Le code JS est extrait tel quel du fichier de référence (aucune réécriture), puis exécuté
// avec de simples bouchons pour les fonctions d'affichage, de son et de progression.
'use strict';
const fs = require('fs');
const path = require('path');

const [, , htmlPath, outDir] = process.argv;
if (!htmlPath || !outDir) {
  console.error('Usage : node tools/export_reference.js <reference.html> <dossier de sortie>');
  process.exit(1);
}
const lines = fs.readFileSync(htmlPath, 'utf8').split('\n');
const find = (start) => {
  const i = lines.findIndex((l) => l.startsWith(start));
  if (i < 0) throw new Error('Déclaration introuvable dans la référence : ' + start);
  return i;
};
// Une ligne seule, ou un bloc jusqu'à la ligne exactement égale à `end`
function grab(start, end) {
  const i = find(start);
  if (!end) return lines[i];
  const j = lines.findIndex((l, k) => k > i && l === end);
  if (j < 0) throw new Error('Fin de bloc introuvable pour : ' + start);
  return lines.slice(i, j + 1).join('\n');
}
// Une fonction entière, en suivant l'équilibre des accolades
function grabFn(start) {
  let i = find(start), depth = 0, seen = false;
  const out = [];
  for (; i < lines.length; i++) {
    out.push(lines[i]);
    for (const ch of lines[i]) {
      if (ch === '{') { depth++; seen = true; } else if (ch === '}') depth--;
    }
    if (seen && depth === 0) return out.join('\n');
  }
  throw new Error('Fonction non terminée : ' + start);
}

const code = [
  grab('const TS=32'), grab('const AIR=0'), grab('const G=22'), grab('const DECK='),
  grab('const LAYERS=[', '];'), grab('const ORES=[', '];'), grab('const ORE_W='), grab('const FOES=[', '];'),
  grab('const MW=W*8'), grab('let M=new Uint8Array'), grab('let hatchOpen='),
  grab('const clamp='), grab('const layerOf='), grab('function hash('), grab('function rng('), grab('function pickW('),
  grab('function chamber('), grab('function mkFoe('), grabFn('function gen('),
  grab('const solid='), grabFn('function tileHP('),
  grab('const cellSolid='), grab('const cellAt='), grabFn('function boxHitsWH('),
  grabFn('function initMask('), grab('const isHatch='), grabFn('function setHatch('), grabFn('function packMask('),
  grabFn('function digAt('), grabFn('function removeCell('),
  // textures
  grab('const PAL=[', '];'), grab('const BEDP='), grab('const hexRgb='), grab('const ORGB='), grab('let chunks=new Map()'), grab('const PX=4'),
  grab('const tileAt='), grab('const tileAt0='), grabFn('function nh('), grabFn('function vnoise('), grabFn('function fbm('),
  grabFn('function texel('), grabFn('function shade('), grabFn('function buildChunk('),
  // foreuse
  grab('const st={', '};'), grab('const P={'), grab('let running=false'), grab('const D45='),
  grab('const onDeck='), grab('const hookedBase='), grab('const atBase='), grab('const inDock='),
  grab('const boxHits='), grabFn('function stepUp('), grabFn('function moveBox('), grabFn('function boxIn('), grabFn('function toSpawn('),
  grabFn('function hurt('), grabFn('function strand('), grabFn('function groundSlope('), grabFn('function aimHead('),
  grabFn('function updPlayer('), grabFn('function armBack('), grabFn('function updArm('), grabFn('function afterMove('), grabFn('function explore('),
  grab('const cargoN='), grabFn('function updNuggets('),
  // économie (addXp renommé : le scénario de forage garde son compteur d'XP simplifié)
  grab('const UPG={', '};'), grab('const TAL={', '};'), grab('const fmt='), grab('const cargoVal='), grab('const barsVal='),
  grab('const cost='), grab('const xpNeed='), grab('const points='), grabFn('function addXp(').replace('function addXp(', 'function realAddXp('),
  grabFn('function sellAll('), grabFn('function smeltAll('), grabFn('function buy('), grabFn('function updFurnace('),
  grab('const OCT='), grab('const dotN='), grab('const BOSSES=[', '];'), grabFn('function foeNear('), grabFn('function bossInFront('),
  grabFn('function hitFoe('), grabFn('function hitBoss('), grabFn('function killBoss('), grabFn('function resetBoss('), grabFn('function updFoes('),
  grab('function bounds('), grab('function newGoal('), grab('function inCh('), grabFn('function updBoss('), grabFn('function updBullets('),
  grab('const b64='), grabFn('function fresh('), grabFn('function save(').replace('function save(', 'function realSave('),
  // bouchons : ni affichage ni son ; on enregistre l'XP et les pépites libérées ; un canevas réduit à son tableau de pixels ;
  // commandes de la foreuse scriptées (pas de souris réelle, ni créature) ; le bras vise le curseur scripté comme armAim()
  'let T, T0, E, xp=0, freed=[], foes=[], bosses=[], bullets=[], parts=[], nuggets=[];',
  'let S={up:{power:0,blade:0,speed:0,tank:0,hull:0,cargo:0,light:0,radar:0,smelt:0,arm:0},tal:{pros:0,luck:0,turbo:0,eco:0,armor:0},cargo:{},bars:{},coins:0,xp:0,lvl:1,furnace:{q:[],p:0,last:0},maxDepth:0,seenL:0,fallTip:0};',
  'function paint(){} function recolorTile(){} function burst(){} function sfx(){} function vib(){} function toast(){} function shake(){}',
  'function save(){} function float(){} function tileColor(){}',
  'function addXp(n){xp+=n} function spawnNugget(id,x,y){freed.push([id,x,y])}',
  'const cur={ix:0,iy:0,c:null,drill:false,grab:false};',
  'let saved=null; const localStorage={setItem:(k,v)=>{saved=[k,v]}};',
  'const inputDir=()=>[cur.ix,cur.iy], cursorVec=()=>cur.c, digOn=()=>cur.drill, grabOn=()=>cur.grab;',
  'function armAim(ix,iy){const c=cur.c;if(c&&c[2]>.3)return[c[0]/c[2],c[1]/c[2]];if(ix||iy){const d=Math.hypot(ix,iy);return[ix/d,iy/d]}return[0,-1]}',
  'const document={createElement:()=>({getContext:()=>({createImageData:(w,h)=>({data:new Uint8ClampedArray(w*h*4)})})})};',
  ';({gen, W, H, SURF, PH, PW, chamber, initMask, digAt, setHatch, buildChunk, setT:(t)=>{T=t;T0=t.slice()}, setTexSeed:(s)=>{texSeed=s},',
  '   getM:()=>M, getR:()=>R, getT:()=>T, getXp:()=>xp, freed, S, P, st, cur, updPlayer, setE:(e)=>{E=e}, getE:()=>E, tick:(dt)=>{time+=dt},',
  '   updNuggets, getNug:()=>nuggets, setNug:(a)=>{nuggets=a},',
  '   UPG, TAL, cost, xpNeed, points, realAddXp, sellAll, smeltAll, buy, updFurnace, cargoVal, barsVal, DECK,',
  '   fresh, realSave, setS:(s)=>{S=s}, getSaved:()=>saved,',
  '   rng, BOSSES, setFoes:(a)=>{foes=a}, getFoes:()=>foes, setBosses:(a)=>{bosses=a}, getBosses:()=>bosses, getBullets:()=>bullets,',
  '   foeNear, bossInFront, hitFoe, hitBoss, updFoes, updBoss, updBullets})',
].join('\n');
const ref = eval(code); // eslint-disable-line no-eval

// FNV-1a 64 bits sur des octets, en hexadécimal (même calcul côté C++)
function fnv(bytes) {
  let h = 0xcbf29ce484222325n;
  for (const b of bytes) h = BigInt.asUintN(64, (h ^ BigInt(b)) * 0x100000001b3n);
  return h.toString(16).padStart(16, '0');
}

fs.mkdirSync(outDir, { recursive: true });

// 1. Mondes générés
for (const seed of [1, 123456789, 2147483647]) {
  const { t, foes } = ref.gen(seed);
  if (t.length !== ref.W * ref.H) throw new Error('taille inattendue');
  fs.writeFileSync(path.join(outDir, `world_${seed}.bin`), Buffer.from(t));
  fs.writeFileSync(path.join(outDir, `foes_${seed}.txt`), foes.map((f) => `${f.k} ${f.i} ${f.tx} ${f.ty}`).join('\n') + '\n');
  console.log(`monde, graine ${seed} : ${t.length} cases, ${foes.length} créatures`);
}

// 2. Scénario de forage sur le monde de la graine 1
{
  const w = ref.gen(1);
  ref.setT(w.t);
  ref.initMask(null);
  const out = [`fnv mask_init ${fnv(ref.getM())}`];
  const dig = (cx, cy, r, b) => {
    const o = ref.digAt(cx, cy, r, b);
    out.push(`dig ${cx} ${cy} ${r} ${b} ${o.removed} ${o.left} ${o.budget} ${o.blocked} ${o.t} ${o.tx} ${o.ty}`);
  };
  for (let k = 0; k < 40; k++) dig(17.5, 6.3 + k * 0.25, 0.47, 3);                 // puits vertical, petit budget
  for (let k = 0; k < 30; k++) dig(10.5 + k * 0.3, 8 + k * 0.3, 0.47, 5);           // galerie en biais
  for (let k = 0; k < 60; k++) dig(3 + k * 0.5, 150 + (k % 7) * 0.4, 0.47, 50);    // bande de la couche 1, gros budget (filons)
  for (let k = 0; k < 10; k++) dig(0.3, 40 + k, 0.47, 50);                         // socle : bloque
  const c0 = ref.chamber(0);
  for (let k = 0; k < 10; k++) dig(8 + k * 2, c0.y2 + 1.5, 0.47, 50);              // sceau : bloque
  dig(17.5, 3, 0.47, 50);                                                          // ciel et pont : ignorés
  out.push(`xp ${ref.getXp()}`);
  for (const [id, x, y] of ref.freed) out.push(`freed ${id} ${x} ${y}`);
  out.push(`fnv mask ${fnv(ref.getM())}`, `fnv remaining ${fnv(ref.getR())}`, `fnv tiles ${fnv(ref.getT())}`);
  ref.setHatch(true);
  out.push(`fnv hatch_open ${fnv(ref.getM())}`);
  ref.setHatch(false);
  out.push(`fnv hatch_closed ${fnv(ref.getM())}`);
  fs.writeFileSync(path.join(outDir, 'dig_1.txt'), out.join('\n') + '\n');
  console.log(`forage, graine 1 : ${out.filter((l) => l.startsWith('dig')).length} coups de foret, ${ref.freed.length} pépites, xp ${ref.getXp()}`);

  // 3. Blocs de texture, après ce forage (galeries creusées, filons extraits, trappe fermée)
  ref.setTexSeed(1 % 9973 + 1);
  const list = [[0, 0], [1, 0], [2, 0], [1, 1], [1, 18], [2, 18], [0, 30], [2, 45]];
  const bufs = list.map(([cx, cy]) => Buffer.from(ref.buildChunk(cx, cy).img.data));
  fs.writeFileSync(path.join(outDir, 'chunks_1.txt'), list.map(([cx, cy]) => `${cx} ${cy}`).join('\n') + '\n');
  fs.writeFileSync(path.join(outDir, 'chunks_1.bin'), Buffer.concat(bufs));
  console.log(`textures, graine 1 : ${list.length} blocs de ${bufs[0].length} octets`);
}

// 4. Parcours de la foreuse, image par image, sur un monde neuf de la graine 1
{
  const w = ref.gen(1);
  ref.setT(w.t);
  ref.initMask(null);
  const E = new Uint8Array(ref.W * ref.H);
  for (let x = 0; x < ref.W; x++) E[ref.SURF * ref.W + x] = 1;
  ref.setE(E);
  const { P, S, st, cur } = ref;
  S.up.power = 2;
  P.x = 17.11; P.y = ref.SURF - ref.PH - 0.001; P.fuel = st.fuel(); P.hull = st.hull();
  const dt = 1 / 60;
  // [images, ix, iy, curseur (dx, dy) ou null, forage]
  const segs = [
    [30, 0, 0, null, false],            // repos au sol
    [120, 0, 0, [0, 1], true],          // curseur sous la foreuse, forage : tête à −45°, trou devant
    [240, 1, 1, [0.707, 0.707], true],  // rampe en descendant vers la droite
    [60, 0, -1, [0, -1], false],        // propulseur
    [120, -1, 0, [-1, 0], true],        // demi-tour et forage vers la gauche
    [60, 0, 0, null, false],            // repos
    [60, 1, 0, null, false],            // roule vers la droite
  ];
  const out = [`stats ${st.dps()} ${st.spd()} ${st.fuel()} ${st.hull()} ${st.light()} ${st.arm()}`];
  for (const [n, ix, iy, c, drill] of segs) out.push(`seg ${n} ${ix} ${iy} ${c ? 1 : 0} ${c ? c[0] : 0} ${c ? c[1] : 0} ${drill ? 1 : 0}`);
  let frame = 0;
  for (const [n, ix, iy, c, drill] of segs) {
    Object.assign(cur, { ix, iy, c: c ? [c[0], c[1], Math.hypot(c[0], c[1])] : null, drill });
    for (let k = 0; k < n; k++) {
      ref.tick(dt);
      ref.updPlayer(dt);
      out.push(`f ${++frame} ${P.x} ${P.y} ${P.vx} ${P.vy} ${+P.grounded} ${P.side} ${P.ang} ${P.th} ${P.tilt} ${P.yaw} ${P.fuel} ${P.hull} ${+P.thrust} ${+P.sliding} ${P.digAcc}`);
    }
  }
  out.push(`fnv mask ${fnv(ref.getM())}`, `fnv explored ${fnv(ref.getE())}`);
  fs.writeFileSync(path.join(outDir, 'drill_1.txt'), out.join('\n') + '\n');
  console.log(`foreuse, graine 1 : ${frame} images, arrivée en (${P.x.toFixed(2)}, ${P.y.toFixed(2)}), carburant ${P.fuel.toFixed(1)}`);
}

// 5. Pépites : chute, rebonds, roulement dans des cratères, aimant et ramassage (vitesses de départ fixées)
{
  const w = ref.gen(1);
  ref.setT(w.t);
  ref.initMask(null);
  const { P, S, st } = ref;
  for (const x of [24, 27, 30]) ref.digAt(x, 6.1, 0.47, 50);   // cratères dans le sol, à droite de la base
  P.x = 2; P.y = ref.SURF - ref.PH - 0.001;                   // foreuse loin des pépites
  S.cargo = {};
  const nug = [];
  for (let k = 0; k < 12; k++) nug.push({ id: 10 + (k % 9), x: 22.3 + k * 0.95, y: 4 + (k % 3) * 0.5, vx: ((k * 37) % 11 - 5) * 0.4, vy: -((k * 13) % 7) * 0.5 });
  ref.setNug(nug);
  const out = [`cargo ${st.cargo()}`];
  for (const n of nug) out.push(`spawn ${n.id} ${n.x} ${n.y} ${n.vx} ${n.vy}`);
  const dt = 1 / 60;
  const phase = (name, frames, setup) => {
    setup();
    out.push(`phase ${name} ${frames} ${P.x} ${P.y} ${JSON.stringify(S.cargo).replace(/\s/g, '')}`);
    for (let f = 0; f < frames; f++) {
      ref.tick(dt);
      ref.updNuggets(dt);
      const total = Object.values(S.cargo).reduce((a, b) => a + b, 0);
      out.push(`n ${ref.getNug().length} ${total} ` + ref.getNug().map((n) => `${n.id} ${n.x} ${n.y} ${n.vx} ${n.vy}`).join(' '));
    }
  };
  phase('chute', 240, () => {});
  phase('pleine', 60, () => { S.cargo = { 10: 10 }; P.x = 26.6; P.y = ref.SURF - ref.PH - 0.001 + 0.0; });
  phase('ramassage', 180, () => { S.cargo = {}; });
  fs.writeFileSync(path.join(outDir, 'nuggets_1.txt'), out.join('\n') + '\n');
  console.log(`pépites, graine 1 : ${nug.length} lancées, ${ref.getNug().length} restantes, soute ${JSON.stringify(S.cargo)}`);
}

// 6. Bras d'accroche dans la salle du gardien de la couche 0 : prise en tombant, rappel, treuil, balancement, lâcher, plafond
{
  const w = ref.gen(1);
  ref.setT(w.t);
  ref.initMask(null);
  const E = new Uint8Array(ref.W * ref.H);
  ref.setE(E);
  const { P, S, st, cur } = ref;
  S.up.power = 0; S.up.arm = 2; S.cargo = {};
  const c = ref.chamber(0);
  Object.assign(P, { x: c.x1 + 0.1, y: c.y1 + 0.05, vx: 0, vy: 0, side: 1, ang: 0, th: 0, thT: 0, yaw: 1, yawT: 1, tilt: 0, slope: 0,
    sliding: false, thrust: false, drilling: false, grounded: false, digAcc: 0, digSnd: 0, hurtCd: 0, ramCd: 0, air: 0, vis: 0, away: 0, arm: null });
  P.fuel = st.fuel(); P.hull = st.hull();
  const dt = 1 / 60;
  const segs = [
    [90, 0, 0, [-1.5, -0.3], true],  // tombe en accrochant la paroi gauche
    [60, 0, 1, [0, 3], true],        // rappel
    [60, 0, -1, [0, -3], true],      // treuil
    [60, 1, 0, [3, 0], true],        // balancement
    [90, 0, 0, null, false],         // lâcher : chute, le bras rentre
    [90, 0, -1, [0, -3], true],      // vise le plafond : prise puis treuil
  ];
  const armState = { out: 1, hooked: 2, back: 3 };
  const out = [`stats ${st.dps()} ${st.spd()} ${st.fuel()} ${st.hull()} ${st.light()} ${st.arm()}`, `start ${P.x} ${P.y}`];
  for (const [n, ix, iy, cc, grab] of segs) out.push(`seg ${n} ${ix} ${iy} ${cc ? 1 : 0} ${cc ? cc[0] : 0} ${cc ? cc[1] : 0} ${grab ? 1 : 0}`);
  let frame = 0;
  for (const [n, ix, iy, cc, grab] of segs) {
    Object.assign(cur, { ix, iy, c: cc ? [cc[0], cc[1], Math.hypot(cc[0], cc[1])] : null, drill: false, grab });
    for (let k = 0; k < n; k++) {
      ref.tick(dt);
      ref.updPlayer(dt);
      const a = P.arm;
      out.push(`f ${++frame} ${P.x} ${P.y} ${P.vx} ${P.vy} ${+P.grounded} ${P.hull} ${P.fuel} ${+P.thrust} ` +
        (a ? `${armState[a.state]} ${a.len ?? 0} ${a.ax ?? 0} ${a.ay ?? 0} ${a.hx ?? 0} ${a.hy ?? 0} ${a.rope ?? 0}` : '0 0 0 0 0 0 0'));
    }
  }
  fs.writeFileSync(path.join(outDir, 'arm_1.txt'), out.join('\n') + '\n');
  console.log(`bras, graine 1 : ${frame} images, coque ${P.hull.toFixed(1)}, arrivée en (${P.x.toFixed(2)}, ${P.y.toFixed(2)})`);
}

// 7. Économie : coûts, effets des améliorations, XP, achats, fonderie (horloge simulée) et vente
{
  const { S, st, P } = ref;
  const keys = Object.keys(ref.UPG), tals = Object.keys(ref.TAL);
  const out = [];
  const reset = () => {
    for (const k of keys) S.up[k] = 0;
    for (const k of tals) S.tal[k] = 0;
    Object.assign(S, { coins: 0, xp: 0, lvl: 1, cargo: {}, bars: {}, furnace: { q: [], p: 0, last: 0 } });
  };
  reset();
  for (const k of keys) {
    for (let l = 0; l <= ref.UPG[k].max; l++) { S.up[k] = l; out.push(`cost ${k} ${l} ${ref.cost(k)}`); }
    S.up[k] = 0;
  }
  const configs = [
    [{}, {}],
    [{ power: 5, blade: 3, speed: 4, tank: 6, hull: 7, cargo: 8, light: 2, radar: 3, smelt: 4, arm: 5 }, { pros: 2, luck: 1, turbo: 3, eco: 4, armor: 5 }],
    [Object.fromEntries(keys.map((k) => [k, ref.UPG[k].max])), Object.fromEntries(tals.map((k) => [k, 5]))],
  ];
  configs.forEach(([up, tal], i) => {
    reset(); Object.assign(S.up, up); Object.assign(S.tal, tal);
    out.push(`config ${i} ${keys.map((k) => S.up[k]).join(' ')} ${tals.map((k) => S.tal[k]).join(' ')}`);
    const vals = [0, 1, 2, 3, 4, 5, 6, 7, 8].map((j) => st.val(j));
    out.push(`stat ${i} ${st.dps()} ${st.ram()} ${st.spd()} ${st.fuel()} ${st.hull()} ${st.cargo()} ${st.light()} ${st.radar()} ${st.smelt()} ${st.arm()} ${vals.join(' ')}`);
  });
  for (let l = 1; l <= 60; l++) out.push(`xpneed ${l} ${ref.xpNeed(l)}`);

  // scénario à la base
  reset();
  Object.assign(P, { x: 19.4, y: ref.DECK - ref.PH - 0.001, grounded: true, arm: null });
  ref.realAddXp(1000.5);
  out.push(`addxp 1000.5 ${S.lvl} ${S.xp} ${ref.points()}`);
  S.coins = 5000;
  for (const k of ['power', 'power', 'tank', 'hull', 'radar', 'arm', 'arm', 'arm', 'arm', 'arm', 'arm', 'arm', 'cargo', 'power'])
    { ref.buy(k); out.push(`buy ${k} ${S.coins} ${S.up[k]}`); }
  const realNow = Date.now;
  let now = 1.7e12;
  Date.now = () => now;
  S.furnace = { q: [], p: 0, last: now };
  S.cargo = { 10: 3, 11: 2, 14: 5 };
  ref.smeltAll();
  out.push(`smelt ${JSON.stringify(S.furnace.q)}`);
  for (const step of [1000, 2500, 7000, 400, 3600000 * 9]) {
    now += step;
    ref.updFurnace();
    out.push(`furnace ${step} ${S.furnace.q.map(([id, n]) => id + ':' + n).join(',') || '-'} ${S.furnace.p} ${[10, 11, 12, 13, 14, 15, 16, 17, 18].map((id) => S.bars[id] || 0).join(' ')}`);
  }
  Date.now = realNow;
  S.cargo = { 12: 4, 16: 1 };
  const before = S.coins;
  ref.sellAll();
  out.push(`sell ${S.coins - before} ${S.coins}`);
  fs.writeFileSync(path.join(outDir, 'economy.txt'), out.join('\n') + '\n');
  console.log(`économie : ${out.filter((l) => l.startsWith('cost')).length} coûts, niveau ${S.lvl} après 1000,5 XP, vente ${S.coins - before} pièces`);
}

// 8. Sauvegarde : une partie enregistrée par la vraie fonction save(), à relire côté C++
{
  const seed = 123456789;
  const s = ref.fresh();
  Object.assign(s, { seed, coins: 4321.5, xp: 17.25, lvl: 3, boss: [1, 0, 0, 0], dead: [2, 5, 11], maxDepth: 37, play: 1234.5678, seenL: 1, fallTip: 1 });
  s.furnace = { q: [[12, 3], [15, 1]], p: 0.75, last: 1700000123456 };
  s.cargo = { 10: 4, 13: 1 };
  s.bars = { 11: 2, 18: 1 };
  Object.assign(s.up, { power: 3, tank: 2, arm: 1 });
  Object.assign(s.tal, { luck: 2 });
  s.set = { music: 0.3, sfx: 1, vib: false, stick: 'dyn' };
  ref.setS(s);
  const w = ref.gen(seed);
  ref.setT(w.t);
  ref.initMask(null);
  const E = new Uint8Array(ref.W * ref.H);
  for (let x = 0; x < ref.W; x++) E[ref.SURF * ref.W + x] = 1;
  for (let y = ref.SURF; y < ref.SURF + 12; y++) for (let x = 14; x < 22; x++) E[y * ref.W + x] = 1;
  ref.setE(E);
  for (const [cx, cy] of [[17.5, 6.4], [17.9, 7.3], [18.6, 8.2], [19.1, 9.6], [19.4, 11.2], [15.2, 10.1]]) ref.digAt(cx, cy, 0.47, 200);
  Object.assign(ref.P, { x: 18.83125, y: 10.400000001, fuel: 57.123456789, hull: 41.5 });
  ref.setNug([{ id: 12, x: 3.14159, y: 8.005, vx: 0, vy: 0 }, { id: 17, x: 20.125, y: 40.335, vx: 1, vy: 2 }, { id: 10, x: 1.005, y: 7, vx: 0, vy: 0 }]);
  ref.realSave();
  const [key, json] = ref.getSaved();
  fs.writeFileSync(path.join(outDir, 'save_js.json'), json);
  fs.writeFileSync(path.join(outDir, 'save_js.txt'), [`key ${key}`, `fnv remaining ${fnv(ref.getR())}`, `fnv tiles ${fnv(ref.getT())}`, `fnv explored ${fnv(ref.getE())}`].join('\n') + '\n');
  console.log(`sauvegarde : ${json.length} caractères, clé ${key}`);
}

// 9. Créatures et gardiens. Math.random est remplacé par la même suite que Rng(4242) côté C++ (attentes des créatures comprises).
// La foreuse reste immobile aux endroits choisis ; à chaque image paire après la 300e, elle frappe dans l'axe (ux, uy) écrit dans le fichier.
{
  const S = ref.fresh();  // avant le remplacement : fresh() tire la graine avec Math.random
  const realRandom = Math.random;
  Math.random = ref.rng(4242);
  const w = ref.gen(1);
  ref.setT(w.t);
  ref.initMask(null);
  ref.setFoes(w.foes);
  ref.setBosses([0, 1, 2, 3].map((L) => { const c = ref.chamber(L), B = ref.BOSSES[L]; return { L, ch: c, x: (c.x1 + c.x2 + 1) / 2, y: c.y2 + 1 - 1.25, hp: B.hp, max: B.hp, dead: false, active: false, state: 'roam', t: 0, gx: 0, vx: 0, vy: 0, flash: 0 }; }));
  S.up.hull = 6; S.tal.armor = 2;
  ref.setS(S);
  const P = ref.P, st = ref.st, dt = 1 / 60, xp0 = ref.getXp();
  const place = (x, y) => { P.x = x; P.y = y; P.tx = Math.floor(P.x + ref.PW / 2); P.ty = Math.floor(P.y + ref.PH / 2); };
  const f0 = w.foes[0];
  place(f0.tx - 0.9, f0.ty + 1 - ref.PH - 0.001);
  Object.assign(P, { vx: 0, vy: 0, hull: st.hull(), fuel: st.fuel(), hurtCd: 0, ramCd: 0 });
  const c1 = ref.chamber(1);
  const out = [`foes ${w.foes.length} foe0 ${f0.tx} ${f0.ty} chamber1 ${c1.x1} ${c1.x2} ${c1.y1} ${c1.y2}`];
  const N = 2400;
  for (let fr = 0; fr < N; fr++) {
    if (fr === 700) { S.up.hull = 12; S.up.blade = 12; place(c1.x1 + 1.5, c1.y2 + 1 - ref.PH - 0.001); P.hull = st.hull(); }  // salle du gardien 2
    if (fr === 1300) place(19.4, ref.DECK - ref.PH - 0.001);   // la foreuse quitte la salle : le gardien se rendort
    if (fr === 1310) place(c1.x1 + 1.5, c1.y2 + 1 - ref.PH - 0.001);
    P.ramCd -= dt; P.hurtCd -= dt;
    const pcx = P.x + ref.PW / 2, pcy = P.y + ref.PH / 2;
    let ux = 1, uy = 0, best = 1e9;
    for (const f of ref.getFoes()) if (f.alive) { const dx = f.x + 0.35 - pcx, dy = f.y + 0.3 - pcy, d = dx * dx + dy * dy; if (d < best) { best = d; ux = dx; uy = dy; } }
    const b1 = ref.getBosses()[1];
    if (b1.active && fr >= 1000) { ux = b1.x - pcx; uy = b1.y - pcy; }
    const l = Math.hypot(ux, uy) || 1; ux /= l; uy /= l;
    if (fr % 2 === 0 && fr >= 300) { const f = ref.foeNear(ux, uy), b = !f ? ref.bossInFront(ux, uy) : null; if ((f || b) && P.ramCd <= 0) f ? ref.hitFoe(f) : ref.hitBoss(b); }
    ref.updFoes(dt); ref.updBoss(dt); ref.updBullets(dt);
    let sum = 0, alive = 0;
    for (const f of ref.getFoes()) if (f.alive) { alive++; sum += f.x + 3 * f.y + 7 * f.vx + 11 * f.vy + f.hp + f.wait + f.dir + (f.grounded ? 1 : 0) + f.flash; }
    let bs = 0; for (const u of ref.getBullets()) bs += u.x + 2 * u.y + u.life;
    out.push(`f ${fr} ${ux} ${uy} ${P.x} ${P.y} ${P.hull} ${P.hurtCd} ${P.ramCd} ${S.coins} ${ref.getXp() - xp0} ${S.dead.length} ${alive} ${sum} ${ref.getBullets().length} ${bs}`);
    for (const b of ref.getBosses()) if (b.active || (b.L === 1 && fr >= 700)) out.push(`b ${b.L} ${b.x} ${b.y} ${b.vx} ${b.vy} ${b.hp} ${b.state} ${b.t} ${b.gx} ${b.active ? 1 : 0} ${b.dead ? 1 : 0} ${b.flash}`);
  }
  out.push(`end ${S.boss.join(' ')} ${S.dead.join(',') || '-'} ${fnv(ref.getT())}`);
  Math.random = realRandom;
  fs.writeFileSync(path.join(outDir, 'creatures_1.txt'), out.join('\n') + '\n');
  console.log(`créatures : ${N} images, ${S.dead.length} tuées, gardiens ${S.boss.join('')}, coque ${P.hull.toFixed(1)}, pièces ${S.coins}`);
}
