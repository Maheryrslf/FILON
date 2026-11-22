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
  // bouchons : ni affichage ni son ; on enregistre l'XP et les pépites libérées ; un canevas réduit à son tableau de pixels ;
  // commandes de la foreuse scriptées (pas de souris réelle, ni créature, ni bras)
  'let T, T0, E, xp=0, freed=[], bosses=[], bullets=[], parts=[], nuggets=[];',
  'let S={up:{power:0,blade:0,speed:0,tank:0,hull:0,cargo:0,light:0,radar:0,smelt:0,arm:0},tal:{pros:0,luck:0,turbo:0,eco:0,armor:0},cargo:{},maxDepth:0,seenL:0,fallTip:0};',
  'function paint(){} function recolorTile(){} function burst(){} function sfx(){} function vib(){} function toast(){} function shake(){}',
  'function save(){} function float(){} function resetBoss(){} function tileColor(){}',
  'function addXp(n){xp+=n} function spawnNugget(id,x,y){freed.push([id,x,y])}',
  'const cur={ix:0,iy:0,c:null,drill:false};',
  'const inputDir=()=>[cur.ix,cur.iy], cursorVec=()=>cur.c, digOn=()=>cur.drill, grabOn=()=>false, foeNear=()=>null, bossInFront=()=>null;',
  'const document={createElement:()=>({getContext:()=>({createImageData:(w,h)=>({data:new Uint8ClampedArray(w*h*4)})})})};',
  ';({gen, W, H, SURF, PH, chamber, initMask, digAt, setHatch, buildChunk, setT:(t)=>{T=t;T0=t.slice()}, setTexSeed:(s)=>{texSeed=s},',
  '   getM:()=>M, getR:()=>R, getT:()=>T, getXp:()=>xp, freed, S, P, st, cur, updPlayer, setE:(e)=>{E=e}, getE:()=>E, tick:(dt)=>{time+=dt}})',
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
