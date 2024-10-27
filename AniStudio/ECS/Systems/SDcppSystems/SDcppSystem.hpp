#pragma once

#include "ECS.h"
#include "Engine.hpp"
#include "pch.h"
#include "stable-diffusion.h"

using namespace ECS;

namespace ECS {
class SDCPPSystem : public BaseSystem {
public:
    SDCPPSystem() {
        AddComponentSignature<InferenceComponent>();
        AddComponentSignature<ImageComponent>();
    }

    ~SDCPPSystem() {}

    void Start() override {
    }

    //void Inference(EntityID entityID) {
    //    //mgr = ANI::Core.GetManager();
    //    inferenceComp = &mgr->GetComponent<InferenceComponent>(entityID);

    //    // Prepare model paths (ensure paths are correctly set)
    //    sd_ctx_t *sd_context = new_sd_ctx(
    //        inferenceComp->model_path, inferenceComp->clip_l_path, inferenceComp->txxl_path,
    //        inferenceComp->diffusion_model_path, inferenceComp->vae_path, inferenceComp->taesd_path,
    //        inferenceComp->control_net_path, inferenceComp->lora_model_dir, inferenceComp->embed_dir,
    //        inferenceComp->stacked_id_embed_dir_c_str, inferenceComp->vae_decode_only, inferenceComp->vae_tiling,
    //        inferenceComp->free_params_immediately, inferenceComp->n_threads, inferenceComp->wtype,
    //        inferenceComp->rng_type, inferenceComp->schedule, inferenceComp->keep_clip_on_cpu,
    //        inferenceComp->keep_control_net_cpu, inferenceComp->keep_vae_on_cpu);

    //    // Txt2img function
    //    sd_image_t *image = txt2img(sd_context, "1girl", nullptr, 0, 7.0f, 1.0f, 512, 512, EULER, 20, 0, 1, nullptr,
    //                                0.0f, 0.0f, true, nullptr);

    //    // Save or set the generated image in ImageComponent
    //    // output->SetImageData(image->data, image->width, image->height, image->channel);

    //    // Free resources
    //    free(image);
    //    free_sd_ctx(sd_context);
    //}

    //void Update() {
    //    if (inferenceComp->shouldInference) {
    //        Inference(inferenceComp->GetID());
    //        inferenceComp->shouldInference = false;
    //    }
    //}

private:
    ImageComponent *output = nullptr;
    InferenceComponent *inferenceComp = nullptr;
    EntityManager *mgr = nullptr;
};
} // namespace ECS