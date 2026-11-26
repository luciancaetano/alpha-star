"""stage_engine.py staging is idempotent: a staged flavor dir ends up holding exactly its manifest.

Run: python -m unittest tools/test_stage_engine.py   (also a gate step)
"""
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import stage_engine  # noqa: E402


class PruneTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dest = pathlib.Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def touch(self, *names):
        for name in names:
            (self.dest / name).write_bytes(b"x")

    def test_leftovers_of_removed_features_are_deleted_and_the_manifest_kept(self):
        keep = stage_engine.owned_files("release", "alpha_star.exe")
        self.touch(*keep, "shadercross.exe", "dxcompiler.dll", "dxil.dll")
        (self.dest / "shipping").mkdir()
        (self.dest / "old_dir").mkdir()
        removed = stage_engine.prune(self.dest, keep)
        self.assertEqual(removed, ["dxcompiler.dll", "dxil.dll", "old_dir", "shadercross.exe"])
        self.assertEqual({p.name for p in self.dest.iterdir()}, keep | {"shipping"})

    def test_restaging_an_exact_dir_changes_nothing(self):
        keep = stage_engine.owned_files("shipping", "alpha_star.exe")
        self.touch(*keep)
        self.assertEqual(stage_engine.prune(self.dest, keep), [])
        self.assertEqual(stage_engine.prune(self.dest, keep), [])
        self.assertEqual({p.name for p in self.dest.iterdir()}, keep)

    def test_only_the_release_engine_carries_the_shader_compiler(self):
        self.assertEqual(stage_engine.owned_files("shipping", "alpha_star"), {"alpha_star"})
        self.assertEqual(stage_engine.owned_files("release", "alpha_star"),
                         {"alpha_star", *stage_engine.SHADER_TOOLS})


if __name__ == "__main__":
    unittest.main()
