// Exporte le monde généré par la version JS de référence, pour les tests de parité C++.
// Usage : node tools/export_reference.js docs/reference-js/filon-v17.html tests/data
// Le code JS est extrait tel quel du fichier de référence (aucune réécriture), puis exécuté.
'use strict';
const fs = require('fs');
const path = require('path');

const [, , htmlPath, outDir] = process.argv;
if (!htmlPath || !outDir) {
  console.error('Usage : node tools/export_reference.js <reference.html> <dossier de sortie>');
  process.exit(1);
}
const lines = fs.readFileSync(htmlPath, 'utf8').split('\n');

// Une déclaration : de la ligne qui commence par `start` jusqu'à la ligne exactement égale à `end` (ou la seule ligne).
function grab(start, end) {
  const i = lines.findIndex((l) => l.startsWith(start));
  if (i < 0) throw new Error('Déclaration introuvable dans la référence : ' + start);
  if (!end) return lines[i];
  const j = lines.findIndex((l, k) => k > i && l === end);
  if (j < 0) throw new Error('Fin de bloc introuvable pour : ' + start);
  return lines.slice(i, j + 1).join('\n');
}

const code = [
  grab('const TS=32'), grab('const AIR=0'), grab('const LAYERS=[', '];'), grab('const ORES=[', '];'), grab('const ORE_W='), grab('const FOES=[', '];'),
  'const FW=.7,FH=.6;',
  grab('const clamp='), grab('const layerOf='), grab('function hash('), grab('function rng('), grab('function pickW('),
  grab('function chamber('), grab('function mkFoe('), grab('function gen(', '}'),
  ';({gen, W, H})',
].join('\n');
const { gen, W, H } = eval(code); // eslint-disable-line no-eval

fs.mkdirSync(outDir, { recursive: true });
for (const seed of [1, 123456789, 2147483647]) {
  const { t, foes } = gen(seed);
  if (t.length !== W * H) throw new Error('taille inattendue');
  fs.writeFileSync(path.join(outDir, `world_${seed}.bin`), Buffer.from(t));
  fs.writeFileSync(path.join(outDir, `foes_${seed}.txt`), foes.map((f) => `${f.k} ${f.i} ${f.tx} ${f.ty}`).join('\n') + '\n');
  console.log(`graine ${seed} : ${t.length} cases, ${foes.length} créatures`);
}
