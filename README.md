# Anistudio
An executable editor for image and video diffusion generation and editing projects written in C/C++ by utilizing [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp) backend.

## Purpose
I created this application to leverage C/C++ for a highly modular and parallelized system capable of loading, inferencing, and training machine learning models, all within a unified UI. Existing UI solutions often rely on external tools for refining or editing, or suffer from performance issues due to JavaScript, TypeScript, or Python limitations—resulting in inconsistent rendering, slow loading, and cache misses. Most local open-source tools also require users to manage Python environments, adding complexity. AniStudio is designed to maintain consistent performance under heavy loads, allowing users to multitask traditional media creation with AI-assisted workflows, all in one seamless application.  

## Features
- Entity Component System for modular development with low technical debt
- Dockable Views to personalize your UI/UX experience
- Customizable ImGui Themes
- Image and video inference (StableDiffusion.cpp)
- Cross-platform (Windows, Linux)
- Included UI editor for ease of development

---


##TODO:
- update headers to include the licence disclaimer for clarity. probably something like this:
```
 /*
 * This file is part of AniStudio.
 *
 * It is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For the commercial license, contact 1fizzledorf@gmail.com.
 */
```
- attributions for external libraries and code

## License

This project is dual-licensed under the **GNU Lesser General Public License v3.0 (LGPL-3.0)** and a **commercial license**.

### LGPL-3.0
If you use this project under the LGPL-3.0, you must:
- Provide the source code of any modifications to the LGPL-licensed parts.
- Allow users to replace the LGPL-licensed components with their own versions.

For full details, see the [LICENSE-LGPL-3.0.txt](LICENSE-LGPL-3.0.txt) file.

### Commercial License
If you prefer to use this project under a commercial license, please contact us at [your-email@example.com] to discuss terms. The commercial license allows:
- Static linking of the library in proprietary software.
- No obligation to provide source code.
- Priority support and additional benefits.

For full details, see the [LICENSE-COMMERCIAL.txt](LICENSE-COMMERCIAL.txt) file.