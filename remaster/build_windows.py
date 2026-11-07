from pathlib import Path
import argparse, pathlib, shutil, subprocess, zipfile, concurrent.futures
parser = argparse.ArgumentParser(description="Rebuild Syndicate HD for Windows using LLVM-MinGW")
parser.add_argument("--compiler", required=True, help="Path to LLVM-MinGW clang++.exe")
args = parser.parse_args()
cc = Path(args.compiler).resolve(strict=True)
project = Path(__file__).resolve().parent
build = project / "build"
build.mkdir(exist_ok=True)
with zipfile.ZipFile(project / "FreeSynd-0.8-source.zip") as archive:
    archive.extractall(build)
root = build / "Freesynd-0.8-Source"
shutil.copytree(project / "overlay", root, dirs_exist_ok=True)
(root / "game/version.h").write_text("#pragma once\n#define FS_VERSION_MAJOR 0\n#define FS_VERSION_MINOR 8\n#define FS_VERSION_PATCH 0\n")
out = build
includes=[root/'engine/include',root/'utils/include',root/'kernel/include',root/'game',root/'engine/src/system',root/'engine/src/sound',root/'extern/utfcpp',root/'extern/png/include',root/'extern/sdl2/include',root/'extern/SDL2_image/include',root/'extern/SDL2_mixer/include']
flags=['--target=i686-w64-mingw32','-std=c++20','-O1','-D_DEBUG','-DSYSTEM_SDL','-DHAVE_SDL_MIXER','-DSDL_MAIN_HANDLED','-DUSE_INTRO_OGG','-DUSE_ASSASSINATE_OGG','-DFS_ETC_DIR="."','-DFS_DATA_DIR="."']
for i in includes:flags+=['-I',str(i)]
files=[]
for sub in ['utils/src','engine/src','kernel/src','game']:
 files+=sorted(p for p in (root/sub).rglob('*.cpp') if 'tools' not in p.relative_to(root).parts)
header_mtime=max(p.stat().st_mtime for folder in ['utils/include','engine/include','kernel/include'] for p in (root/folder).rglob('*.h'))
def compile_one(src):
 obj=build/('_'.join(src.relative_to(root).parts)+'.o')
 if obj.exists() and obj.stat().st_mtime>max(src.stat().st_mtime,header_mtime):return obj,None
 result=subprocess.run([str(cc),*flags,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
 return obj,result.stderr if result.returncode else None
objects=[];errors=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
 for obj,error in pool.map(compile_one,files):
  objects.append(obj)
  if error: errors.append(error);print(error[:5000],flush=True)
print('Compiled',len(files),'files; errors:',len(errors),flush=True)
if errors:raise SystemExit(1)
libs=[root/'extern/sdl2/lib/x86/SDL2.lib',root/'extern/SDL2_image/lib/x86/SDL2_image.lib',root/'extern/SDL2_mixer/lib/x86/SDL2_mixer.lib',root/'extern/png/lib/libpng.lib',root/'extern/png/lib/zdll.lib']
link=[str(cc),'--target=i686-w64-mingw32',*[str(o) for o in objects],*[str(p) for p in libs],'-o',str(out/'Syndicate-HD.exe'),'-lwinmm','-limm32','-lole32','-lversion','-lsetupapi','-static-libgcc','-static-libstdc++']
r=subprocess.run(link,capture_output=True,text=True);print(r.stdout,r.stderr,flush=True);raise SystemExit(r.returncode)
