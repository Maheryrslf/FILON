// Assemble la version web en un seul fichier hors ligne (styles et scripts intégrés), pour le téléphone ou un APK.
// Usage : node tools/build_web.js [web/index.html] [web/dist/filon.html]
// Aucune dépendance : chaque <link rel="stylesheet" href> et <script src> de la page est remplacé par le contenu du fichier.
'use strict';
const fs = require('fs');
const path = require('path');

const page = path.resolve(process.argv[2] || 'web/index.html');
const out = path.resolve(process.argv[3] || 'web/dist/filon.html');
const dir = path.dirname(page);

const read = (href) => {
  const file = path.resolve(dir, href);
  if (!file.startsWith(dir + path.sep)) throw new Error(`chemin hors de la page : ${href}`);
  return fs.readFileSync(file, 'utf8').trimEnd();
};

let css = [];
const lines = [];
for (const line of fs.readFileSync(page, 'utf8').split('\n')) {
  const link = line.match(/^<link rel="stylesheet" href="([^"]+)">$/);
  if (link) { css.push(read(link[1])); continue; }
  if (css.length) { lines.push('<style>', css.join('\n'), '</style>'); css = []; }  // styles regroupés dans l'ordre de la page
  const script = line.match(/^<script src="([^"]+)"><\/script>$/);
  if (script) {
    const js = read(script[1]);
    if (/<\/script/i.test(js)) throw new Error(`${script[1]} contient </script>`);
    lines.push('<script>', js, '</script>');
    continue;
  }
  if (/<link |<script src=/.test(line)) throw new Error(`ressource externe non reconnue : ${line}`);
  lines.push(line);
}
fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, lines.join('\n'));
console.log(`${path.relative(process.cwd(), out)} : ${fs.statSync(out).size} octets, un seul fichier, hors ligne`);
