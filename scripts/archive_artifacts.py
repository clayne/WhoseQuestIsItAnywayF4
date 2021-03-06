import argparse
import os
import zipfile
import zlib

def make_rel_archive(a_args):
	archive = zipfile.ZipFile(a_args.name + ".zip", "w", zipfile.ZIP_DEFLATED)
	archive.write(a_args.dll, "F4SE/Plugins/{}".format(os.path.basename(a_args.dll)))

def make_dbg_archive(a_args):
	archive = zipfile.ZipFile(a_args.name + "_pdb" + ".zip", "w", zipfile.ZIP_DEFLATED)
	archive.write(a_args.pdb, os.path.basename(a_args.pdb))

def parse_arguments():
	parser = argparse.ArgumentParser(description="archive build artifacts for distribution")
	parser.add_argument("--dll", type=str, help="the full dll path", required=True)
	parser.add_argument("--name", type=str, help="the project name", required=True)
	parser.add_argument("--pdb", type=str, help="the full pdb path", required=True)
	parser.add_argument("--src-dir", type=str, help="the project root source directory", required=True)
	return parser.parse_args()

def main():
	out = "artifacts"
	try:
		os.mkdir(out)
	except FileExistsError:
		pass
	os.chdir(out)

	args = parse_arguments()
	make_rel_archive(args)
	make_dbg_archive(args)

if __name__ == "__main__":
	main()
