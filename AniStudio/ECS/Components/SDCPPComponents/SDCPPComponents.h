#pragma once

// Sampling and Inference
#include "CFGComponent.hpp"
#include "PromptComponent.hpp"
#include "SamplerComponent.hpp"

// Models
#include "Models/ModelComponent.hpp"
#include "Models/DiffusionModelComponent.hpp"
#include "Models/EsrganComponent.hpp"
#include "Models/LoraComponent.hpp"
#include "Models/ControlnetComponent.hpp"
#include "Models/EmbeddingComponent.hpp"
#include "Models/PhotomakerComponent.hpp"

// Vaes
#include "Vaes/VaeComponent.hpp"
#include "Vaes/TaesdComponent.hpp"

// Encoders
#include "Encoders/ClipGComponent.hpp"
#include "Encoders/ClipLComponent.hpp"
#include "Encoders/T5XXLComponent.hpp"

//Latents
#include "LatentComponent.hpp"
#include "LatentUpscaleComponent.hpp"