"""Validate our authored configuration against the inspected Fallout 4 MCM UI contract."""
import configparser
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / 'Data/MCM/Config/PhotoMode'


class MCMConfig(unittest.TestCase):
    def test_controls_and_defaults(self):
        menu = json.loads((CONFIG / 'config.json').read_text())
        ini = configparser.ConfigParser()
        ini.optionxform = str
        ini.read(CONFIG / 'settings.ini')
        self.assertEqual(menu['modName'], 'PhotoMode')
        self.assertNotIn('pluginRequirements', menu)
        ids = set()
        persisted = set()
        types = {'switcher', 'section', 'text', 'slider', 'dropdown', 'keyinput', 'hotkey'}
        for page in menu['pages']:
            self.assertTrue(page['pageDisplayName'])
            for control in page['content']:
                self.assertIn(control['type'], types)
                if 'id' not in control:
                    continue
                setting = control['id']
                self.assertNotIn(setting, ids)
                ids.add(setting)
                if control['type'] == 'hotkey':
                    continue
                key, section = setting.split(':')
                persisted.add((section, key))
                value = ini[section][key]
                options = control['valueOptions']
                self.assertEqual(options['sourceType'], {
                    'b': 'ModSettingBool', 'i': 'ModSettingInt',
                    'f': 'ModSettingFloat', 's': 'ModSettingString'}[key[0]])
                if control['type'] == 'keyinput':
                    self.assertEqual(options['allowModifierKeys'], 0)
                    code, modifiers = map(int, value.split(','))
                    self.assertTrue(0 <= code < 266)
                    self.assertEqual(modifiers, 0)
                elif control['type'] == 'dropdown':
                    self.assertTrue(0 <= int(value) < len(options['options']))
                    if key.endswith('Controller'):
                        self.assertEqual(len(options['options']), 17)
                elif control['type'] == 'slider':
                    self.assertTrue(options['min'] <= float(value) <= options['max'])
                    self.assertGreater(options['step'], 0)
                elif control['type'] == 'switcher':
                    self.assertIn(value, ('0', '1'))
        self.assertEqual(persisted, {(section, key) for section in ini.sections() for key in ini[section]})
        self.assertFalse(any('ScreenshotIndex' in key for _, key in persisted))

    def test_registered_hotkey_uses_existing_console_command(self):
        bindings = json.loads((CONFIG / 'keybinds.json').read_text())
        menu = json.loads((CONFIG / 'config.json').read_text())
        self.assertEqual(bindings['modName'], menu['modName'])
        actions = {item['id']: item['action'] for item in bindings['keybinds']}
        for page in menu['pages']:
            for control in page['content']:
                if control['type'] == 'hotkey':
                    self.assertEqual(actions[control['id']], {'type': 'RunConsoleCommand', 'command': 'PhotoMode'})
        self.assertEqual(len(actions), 1)
        self.assertFalse((ROOT / 'Data/MCM/Settings').exists(), 'Never ship user overrides or global keybind saves')
