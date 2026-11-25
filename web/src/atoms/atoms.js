// Atomes : les plus petites pièces de l'interface. Fonctions pures, données en entrée, HTML en sortie.
var UI = window.UI || (window.UI = {});

UI.coin = () => '<span class="coin"></span>';

UI.gem = ore => `<i class="gem" style="--c:${ore.c};--g:${ore.g}"></i>`;

UI.note = html => `<p class="note">${html}</p>`;

UI.stats = html => `<div class="stats">${html}</div>`;

// variant : '' | 'big' | 'warn' ; key : donnée envoyée avec l'action (amélioration, talent)
UI.button = ({label, action, key, variant, disabled}) =>
  `<button class="btn${variant ? ' ' + variant : ''}" data-a="${action}"${key ? ` data-k="${key}"` : ''}${disabled ? ' disabled' : ''}>${label}</button>`;

UI.meter = pct => `<div class="meter"><i style="width:${pct}%"></i></div>`;

UI.range = ({id, value}) => `<input id="${id}" type="range" min="0" max="1" step=".05" value="${value}">`;

UI.checkbox = ({id, checked}) => `<input id="${id}" type="checkbox"${checked ? ' checked' : ''}>`;

UI.select = ({id, value, options}) =>
  `<select id="${id}">${options.map(([v, label]) => `<option value="${v}"${v === value ? ' selected' : ''}>${label}</option>`).join('')}</select>`;
