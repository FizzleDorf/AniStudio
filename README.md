AniStudio

An ML Toolbox for realtime media, animations and learning in C++ or using the ComfyUI api.
The purpose of this project is to bring together machine learning tasks from the fields of
audio, images, text, video and 3D generation in an easy to use UI unlike what is currently 
being offered AI front-ends.

##Install

```
git clone https://github.com/FizzleDorf/AniStudio.git
cd AniStudio
git submodule add https://github.com/thedmd/imgui-node-editor.git external/imgui-node-editor
git submodule update --init --recursive
```

##Build

```
mkdir build && cd build && cmake .. && cmake --build .
```

then build using your IDE or using make commands. Submodules aren't implemented yet.

TODO:
- convert IMGUI to a hybrid of immediate and retained modes
- display gui
- implement json saving to IMGUI nodes
- add IMGUI nodes
- implement opengl for linux
- fix ECS issues
- implement ComfyUI api
- attempt event based diffusion
- implement two execution methods:
   - realtime simulation
   - single execution
