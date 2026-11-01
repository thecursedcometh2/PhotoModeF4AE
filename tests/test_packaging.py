import importlib.util
import pathlib
import tempfile
import unittest
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('package_release', ROOT / 'scripts/package_release.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class Packaging(unittest.TestCase):
    def test_layout_and_external_fonts(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            dll = tmp / 'po3_PhotoModeF4.dll'
            pdb = tmp / 'po3_PhotoModeF4.pdb'
            dll.write_bytes(b'synthetic DLL fixture, not a compiled plugin')
            pdb.write_bytes(b'synthetic PDB fixture')
            module.package(dll, pdb, tmp / 'out')
            with zipfile.ZipFile(tmp / 'out/PhotoModeF4-1.11.240.zip') as archive:
                self.assertEqual(archive.read('Data/F4SE/Plugins/' + dll.name), dll.read_bytes())
                self.assertIn('LICENSE.txt', archive.namelist())
                self.assertIn('licenses/CommonLibF4-EXCEPTIONS.txt', archive.namelist())
                self.assertFalse(any('ImGuiFonts' in name or name.endswith('.pdb') for name in archive.namelist()))

    def test_missing_artifact_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            with self.assertRaises(RuntimeError):
                module.package(tmp / 'absent.dll', tmp / 'absent.pdb', tmp / 'out')
