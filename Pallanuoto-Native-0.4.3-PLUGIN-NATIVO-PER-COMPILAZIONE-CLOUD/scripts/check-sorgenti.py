from pathlib import Path
import json,re,sys
root=Path(__file__).resolve().parent.parent
plugin=(root/'src/plugin.cpp').read_text(encoding='utf-8')
js=(root/'data/app.js').read_text(encoding='utf-8')
css=(root/'data/style.css').read_text(encoding='utf-8')
assert json.loads((root/'buildspec.json').read_text())['version']=='0.4.3'
assert json.loads((root/'data/default.json').read_text())
assert 'QFontComboBox' in plugin and 'QCheckBox' in plugin
assert 'captainItem' in plugin and 'Calotta rossa obbligatoria' in plugin
assert 'fontColor' in plugin and 'fontSize' in plugin
assert 'redCap' in js and 'captain-tag' in js
assert '--display-size' in css
print('CHECK SORGENTI OK: 0.4.3 native C++/Qt, UI font, calotte, capitano, CSS')
