// Molécules : petits groupes d'atomes qui forment une unité (ligne d'article, total, réglage, onglets).
var UI = window.UI || (window.UI = {});

// Ligne d'article : titre, précision en petit, description, et à droite un prix ou un bouton
UI.row = ({title, meta, desc, aside}) =>
  `<div class="row"><div><h4>${title}${meta != null ? ` <small>${meta}</small>` : ''}</h4>${desc != null ? `<p>${desc}</p>` : ''}</div>${aside || ''}</div>`;

// Ligne de minerai ou de lingot : losange de couleur, quantité × prix, total
UI.oreRow = ({ore, bar, desc, total}) => UI.row({title: UI.gem(ore) + ore.name, meta: bar ? 'lingot' : null, desc, aside: `<b>${total}</b>`});

UI.sum = ({label, value}) => `<div class="sum"><span>${label}</span>${value != null ? `<b>${value}</b>` : ''}</div>`;

UI.option = ({id, label, control}) => `<label class="opt" for="${id}"><span>${label}</span>${control}</label>`;

// tone : 'confirm' (choix réversible) ou 'danger' (perte de données)
UI.confirmBox = ({tone, text, buttons}) => `<div class="${tone}"><p>${text}</p>${buttons.map(UI.button).join('')}</div>`;

UI.tabBar = ({tabs, current}) =>
  tabs.map(([k, name]) => `<button type="button" role="tab" aria-selected="${k === current}" data-tab="${k}">${name}</button>`).join('') +
  '<button type="button" class="close" data-a="close">Fermer</button>';
