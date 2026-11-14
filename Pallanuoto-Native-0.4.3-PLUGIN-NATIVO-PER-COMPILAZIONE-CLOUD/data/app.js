const $=s=>document.querySelector(s), esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
function resize(){document.querySelector('#stage').style.transform=`scale(${Math.min(innerWidth/1920,innerHeight/1080)})`;}
window.addEventListener('resize',resize);
function logo(t){const rgb=t.color.match(/[a-f0-9]{2}/gi).map(x=>parseInt(x,16));const ink=(rgb[0]*.2126+rgb[1]*.7152+rgb[2]*.0722)>160?"#101114":"#ffffff";return t.logo?`<img class="logo" src="${esc(t.logo)}" alt="">`:`<div class="logo badge" style="--accent:${t.color};color:${ink}">${esc(t.short)}</div>`;}
const fallbackStyles={lineups:{family:'Arial',color:'#ffffff',size:25,bold:false,italic:false},score:{family:'Arial',color:'#ffffff',size:15,bold:true,italic:true},intro:{family:'Arial',color:'#ffffff',size:30,bold:true,italic:true}};
function styleVars(s,key){
 const o=s.typography?.[key]||fallbackStyles[key];
 // I nomi dei font sono convalidati dal plug-in nativo; il fallback preserva i salvataggi vecchi.
 const safeFamily=/^[\p{L}\p{N} ._+\-]{1,80}$/u.test(o.family||'')?o.family:'Arial';
 const color=/^#[0-9a-f]{6}$/i.test(o.color||'')?o.color:'#ffffff';
 const size=Math.max(10,Math.min(60,Number(o.size)||fallbackStyles[key].size));
 return `--display-family:'${esc(safeFamily)}',Arial,sans-serif;--display-color:${color};--display-size:${size}px;--display-weight:${o.bold?'800':'400'};--display-style:${o.italic?'italic':'normal'}`;
}
function redCap(value){const raw=String(value??'').trim();return /^0*(1|13)$/.test(raw);}
let lastScene,transition,lastScores;
function draw(s){const changed=lastScores?[s.homeScore!==lastScores[0],s.awayScore!==lastScores[1]]:[false,false];const sameScore=lastScene==='score'&&s.scene==='score';lastScores=[s.homeScore,s.awayScore];const g=$('#graphic');let html='';
 if(['both','home','away'].includes(s.scene)){
 const keys=s.scene==='both'?['home','away']:[s.scene];
 html=`<div class="lineups graphic ${keys.length===1?'single':''}" style="${styleVars(s,'lineups')}">${keys.map(k=>{const team=s[k],count=team.players.length;return `<div class="fullteam ${count>20?'dense':count>16?'compact':''}" style="--accent:${team.color}"><div class="cap">${k==='home'?'CASA':'OSPITI'} · FORMAZIONE COMPLETA</div><div class="teamhead">${logo(team)}<div><div class="teamname">${esc(team.name)}</div><div class="muted">${count} ATLETE</div></div></div><div class="fullroster" style="--rows:${Math.max(1,count)}">${team.players.map(a=>`<div class="row"><span class="num ${redCap(a.number)?'cap-red':''}">${esc(a.number)}</span><span class="player-name">${esc(a.name)}</span><span class="captain">${a.captain?'<span class="captain-tag">C</span>':''}</span></div>`).join('')||'<div class="row">Nessun atleta</div>'}</div><div class="staff">${esc(team.staff)}</div></div>`;}).join('')}</div>`;
 }
 if(s.scene==='score'){
 const badge=t=>t.logo?logo(t):'<div class="score-placeholder" aria-hidden="true"></div>';
 const number=String(s.period).match(/^\s*(\d+)/);const period=number?number[1]:s.period;
 html=`<div class="scoreboard graphic" style="--home:${s.home.color};--away:${s.away.color};${styleVars(s,'score')}"><div class="score-rows">${['home','away'].map(k=>`<div class="score-line ${k}">${badge(s[k])}<strong>${esc(s[k].name)}</strong><span class="score-value" data-score="${k}">${s[k+'Score']}</span></div>`).join('')}</div><div class="score-period"><small>TEMPO</small><strong class="${number?'':'period-text'}">${esc(period)}</strong></div></div>`;
 }
 if(s.scene==='intro')html=`<div class="intro graphic" style="${styleVars(s,'intro')}">${s.eventLogo?`<img class="event-logo" src="${esc(s.eventLogo)}" alt="Logo manifestazione">`:""}<h1>${esc(s.title)}</h1><div class="versus"><div>${logo(s.home)}<strong>${esc(s.home.name)}</strong></div><b>VS</b><div>${logo(s.away)}<strong>${esc(s.away.name)}</strong></div></div><p>${esc(s.subtitle)}</p></div>`;
 clearTimeout(transition);if(lastScene!==undefined&&lastScene!==s.scene&&g.firstElementChild){g.firstElementChild.classList.add('leaving');transition=setTimeout(()=>g.innerHTML=html,220);}else{g.innerHTML=html;if(lastScene===s.scene)g.firstElementChild?.style.setProperty('animation','none');}if(sameScore){['home','away'].forEach((key,i)=>{if(changed[i])g.querySelector(`[data-score="${key}"]`)?.classList.add('score-bump');});}lastScene=s.scene;
}

fetch('/api/state').then(r=>r.json()).then(s=>{draw(s);resize();});
const events=new EventSource('/events');events.onmessage=e=>draw(JSON.parse(e.data));resize();
