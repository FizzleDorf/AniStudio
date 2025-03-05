#include <iostream>

#include <ImJSchema/ImJSchema.h>

#include <SDL.h>
#include <imgui.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES3/gl3.h>
#else

#endif

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#include <../res/bindings/imgui_impl_sdl2.h>
#include <../res/bindings/imgui_impl_sdlrenderer2.h>

namespace IJS = ImJSchema;



bool BeginFullScreen(const char* name, bool* p_open = nullptr, ImGuiWindowFlags _flags = 0)
{
    static bool use_work_area = true;
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | _flags;

    // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
    // Based on your use case you may want one or the other.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

    return ImGui::Begin(name, p_open, flags);
}



std::string _schemaString = R"foo(
{
    "type": "number"
})foo";
nlohmann::json _schema;
nlohmann::json _value = nlohmann::json::object_t();
nlohmann::json _cache = nlohmann::json::object_t();

constexpr auto basic_number =
R"foo({
    "type": "number"
})foo";

constexpr auto basic_boolean =
R"foo({
    "type": "boolean"
})foo";

constexpr auto basic_string =
R"foo({
    "type": "string"
})foo";

constexpr auto basic_object =
R"foo({
    "type": "object",
    "properties" : {
        "number" : { "type" : "number"},
        "bool"   : { "type" : "boolean"},
        "string" : { "type" : "string"}
    }
})foo";

constexpr auto basic_array = R"foo({
    "type": "array",
    "items" : {
        "type" : "number"
    }
})foo";


constexpr auto descriptions = R"foo({
    "description" : "Each schema object can have a \"description\" property to display visible text",
    "type": "object",
    "ui:order" : ["name", "age", "imguiAwesome", "object"],
    "properties" : {
        "name" : {
            "type" : "string",
            "description" : "Please enter the name wish to be called"
        },
        "age" : {
            "type" : "number",
            "description" : "The age you will be at the end of this year"
        },
        "imguiAwesome" : {
            "type" : "boolean",
            "description" : "Check this box if you think ImGui is awesome."
        },
        "object" : {
            "type" : "object",
            "description" : "Objects within an object type will show up as a Collapsable header",
            "properties" : {
                 "number" : { "type" : "number"}
            }
        }
    }
})foo";


constexpr auto ordering =
R"foo({
    "type": "object",
    "description" : "By default, properties in an object will be displayed in alphabetical order. You can set the order the widgets appear by setting the ui:order property. If you do not list the widget, it will not show up in the list",
    "ui:order" : ["b", "c", "a"],
    "properties": {
        "a": { "type": "string", "ui:help" : "Hover over the label to show this tooltip"},
        "b": { "type": "string" },
        "c": { "type": "string" },
        "d": { "type": "string" }
    }
})foo";

constexpr auto titles =
R"foo({
    "description" : "Each schema object can have a \"title\". This value will show up as the label ",
    "type": "object",
    "ui:order" : ["name", "age", "imguiAwesome", "array", "object"],
    "properties" : {
        "name" : {
            "type" : "string",
            "title" : "Name"
        },
        "age" : {
            "type" : "number",
            "title" : "Age"
        },
        "imguiAwesome" : {
            "type" : "boolean",
            "title" : "Is ImGui Awesome?"
        },
        "object" : {
            "type" : "object",
            "title" : "Super Special Object",
            "properties" : {
                 "number" : { "type" : "number"}
            }
        },
        "array" : {
            "type" : "array",
            "title" : "Super Special Array",
            "items" : {
                 "type" : "number"
            }
        }
    }
})foo";

constexpr auto help =
R"foo({
    "description" : "You can use the \"ui:help\" : \"tool tip text\" to show tooltip. Hover over the object label",
    "type": "object",
    "ui:order" : ["name", "age", "imguiAwesome", "object"],
    "properties" : {
        "name" : {
            "type" : "string",
            "title" : "Name",
            "ui:help" : "Please enter the name wish to be called"
        },
        "age" : {
            "type" : "number",
            "title" : "Age",
            "ui:help" : "The age you will be at the end of this year"
        },
        "imguiAwesome" : {
            "type" : "boolean",
            "title" : "Is ImGui Awesome?",
            "ui:help" : "Check this box if you think ImGui is awesome."
        },
        "object" : {
            "type" : "object",
            "ui:help" : "Objects within an object type will show up as a Collapsable header",
            "properties" : {
                 "number" : { "type" : "number"}
            }
        }
    }
})foo";

constexpr auto number_widgets =
R"foo({
    "type": "object",
    "properties": {
        "float": {
            "type": "number",
            "default": 0.0,
            "ui:step": 1,
            "ui:step_fast": 10,
            "description" : "Default floating point number. if ui:step is not provided, the increment buttons will not show. Default ui:step_fast is 10x ui:step"
        },
        "float_slider": {
            "type": "number",
            "default": 0.0,
            "maximum": 10.0,
            "minimum": 0.0,
            "ui:widget": "slider",
            "description" : "Slider widgets require both a minimum and maximum property to be set. Will default to drag widget if either of these are not provided"
        },
        "float_drag": {
            "type": "number",
            "default": 0.0,
            "maximum": 1.0,
            "minimum": 0.0,
            "ui:speed": 0.0010000000474974513,
            "ui:widget": "drag",
            "description" : "Drag widgets"
        },
        "int": {
            "type": "integer",
            "default": 0,
            "ui:step": 1,
            "ui:step_fast": 10,
            "description" : "Integer widgets can be used as well, by setting the type to \"integer\""
        },
        "int_drag": {
            "type": "integer",
            "default": 0,
            "maximum": 10,
            "minimum": 0,
            "ui:step": 1,
            "ui:step_fast": 10,
            "ui:widget": "drag"
        },
        "int_slider": {
            "type": "integer",
            "default": 0,
            "maximum": 10,
            "minimum": 0,
            "ui:step": 1,
            "ui:step_fast": 10,
            "ui:widget": "slider"
        }
    }
})foo";

constexpr auto boolean_widgets =
R"foo({
    "description" : "Boolean widgets. This object makes use of the \"ui:column_size\" property.",
    "ui:column_size" : 50,
    "type": "object",
    "properties": {
        "checkbox": {
            "type": "boolean",
            "default": false,
            "title" : "Check Box",
            "description" : "The default is checkbox"
        },
        "enabledisable": {
            "type": "boolean",
            "default": false,
            "ui:widget": "enabledisable",
            "title" : "Enable/Disable",
            "description" : "But there are also different styles. See Enumerated Types for an alternative method"
        },
        "truefalse": {
            "type": "boolean",
            "default": false,
            "ui:widget": "truefalse",
            "title" : "True/False"
        },
        "yesno": {
            "type": "boolean",
            "default": true,
            "ui:widget": "yesno",
            "title" : "Yes/No"
        }
    }
})foo";

constexpr auto string_widgets =
R"foo({
    "type": "object",
    "ui:order" : ["basic", "textarea", "color", "color_picker"],
    "properties": {
        "basic": {
            "type": "string"
        },
        "color": {
            "type": "string",
            "ui:widget": "color"
        },
        "color_picker": {
            "type": "string",
            "ui:widget": "color_picker"
        },
        "textarea": {
            "type": "string",
            "ui:options": {
                "rows": 5
            },
            "ui:widget": "textarea"
        }
    }
})foo";

constexpr auto array_widgets =
R"foo({
    "type": "object",
    "ui:order" : ["fixed_length", "varying_size", "color_array", "color_array_alpha"],
    "properties": {
        "fixed_length": {
            "type": "array",
            "items" : {
                "type": "string",
                "default": "hello world"
            },
            "minItems" : 3,
            "maxItems" : 3,
            "title" : "Fixed Length Array",
            "description" : "You can set the number of items in the array using the minItems and maxItems properties"
        },
        "varying_size": {
            "type": "array",
            "items" : {
                "type": "string",
                "default": "hello world"
            },
            "title" : "Varying Length Array",
            "description" : "If you dont set the minItems/maxItems, you can grow or shrink the array using the buttons."
        },
        "color_array": {
            "type": "array",
            "items" : {
                "type": "number"
            },
            "default" : [1,0.5,0.7],
            "maxItems" : 3,
            "minItems" : 3,
            "ui:widget" : "color",
            "title" : "Color Array",
            "description" : "Setting items == number, will give values ranging from 0-1. If minItems == maxItems == 3, then you will get a RGB color picker."
        },
        "color_array_alpha": {
            "type": "array",
            "items" : {
                "type": "integer"
            },
            "default" : [128, 200, 96, 255],
            "maxItems" : 4,
            "minItems" : 4,
            "ui:widget" : "color",
            "title" : "Color Array Alpha",
            "description" : "Setting items == items, will give values ranging from 0-255. If minItems == maxItems == 4, then you will get a RGBA color picker."
        }
    }
})foo";


constexpr auto object_widgets =
    R"foo({
    "type": "object",
    "ui:order" : ["regular", "header", "collapsing"],
    "properties": {
        "regular": {
            "type": "object",
            "title" : "Regular Object",
            "properties" :
            {
                "string" : { "type" : "string" },
                "number" : { "type" : "number" },
                "boolean" : { "type" : "boolean" }
            },
            "description" : "This is a regular object type. If ui:resizable is true, then you will be able to resize the label field"
        },
        "header": {
            "type": "object",
            "title" : "Header Widget",
            "ui:widget" : "header",
            "properties" :
            {
                "string" : { "type" : "string" },
                "number" : { "type" : "number" },
                "boolean" : { "type" : "boolean" }
            },
            "description" : "Objects with ui:widget=header, will show up as headers"
        },
        "collapsing": {
            "type": "object",
            "title" : "Collapsing Widget",
            "ui:widget" : "collapsing",
            "properties" :
            {
                "string" : { "type" : "string" },
                "number" : { "type" : "number" },
                "boolean" : { "type" : "boolean" }
            },
            "description" : "Objects with ui:widget=collapsing, will show up as collapsable headers"
        }
    }
})foo";



constexpr auto constants =
R"foo({
    "type": "object",
    "description" : "You can hide constant values in the form by setting the 'default', 'ui:hidden', and 'ui:disabled' properties.",
    "properties": {
        "constant_value": {
            "type": "string",
            "default" : "This is the default value",
            "ui:hidden" : true,
            "ui:disabled" : true
        },
        "b": { "type": "string" },
        "c": { "type": "string" }
    }
})foo";


constexpr auto enumerated_types =
R"foo({
    "type": "object",
    "description" : "Enumerated types allow you to choose from one option. It requires the \"enum\" property. Additinally, if provided, the \"enumNames\" property will be used for displaying",
    "properties": {
        "strings": {
            "type": "string",
            "enum" : ["wiz", "src", "war"]
        },
        "strings_with_names": {
            "type": "string",
            "title" : "String with Names",
            "default" : "rogue",
            "description" : "When using \"ui:widget\" : \"buttons\", You can set \"ui:options/columns\" to be a number to set the total number of columns for the buttons ",
            "enum" : ["wiz", "sorc", "war", "fight", "barb", "art", "rogue", "monk", "pal"],
            "enumNames" : ["Wizard", "Sorcerer", "Warlock", "Fighter", "Barbarian", "Artificer", "Rogue", "Monk", "Paladin"],
            "ui:widget" : "button",
            "ui:options" : {
               "columns" : 3
            }
        },
        "numbers": {
            "type": "number",
            "title" : "Numbers",
            "enum" : [1,2,3]
        },
        "numbers_with_names": {
            "type": "number",
            "title" : "Numbers With Names",
            "enum" : [1,2,3],
            "enumNames" : ["first", "second", "third"]
        },
        "booleans": {
            "type": "boolean",
            "title" : "boolean",
            "enum" : [false, true]
        },
        "booleans_with_names": {
            "type": "boolean",
            "title" : "Boolean with Names",
            "enum" : [false, true],
            "enumNames" : ["False", "True"]
        }
    }
})foo";

constexpr auto references =
    R"foo({
    "$defs": {
        "number": {
            "default": 0.75,
            "type": "number"
        },
        "normalized": {
            "maximum": 1.0,
            "minimum": 0.0,
            "default": 0.33,
            "ui:widget" : "slider"
        },
        "enum_list" : ["Hello", "this", "is", "a", "list"]
    },
    "description" : "References allow you to reuse parts of your schema.\n\nThe drawSchemaWidget( ) function does not natively support the $ref property.\n\nIf you have a schema object, J,  you can use the jsonExpandAllReferences(J) function\nto expand the references before you pass it into the drawSchemaWidget()",
    "ui:order" : ["single_ref", "multi_ref", "enum_list"],
    "properties" : {
        "single_ref" : {
            "description" : "The $ref string property points to a path within the JSON document",
            "$ref": "#/$defs/number"
        },
        "multi_ref" : {
            "description" : "You can also use multiple references as a string array. Earlier items are overridden by later values in the array",
            "$ref": ["#/$defs/number", "#/$defs/normalized"]
        },
        "enum_list" : {
            "description" : "Any property can set as a reference to a definition",
            "type" : "string",
            "enum" : { "$ref" : "#/$defs/enum_list"}
        }
    },
    "type": "object"
})foo";

constexpr auto DnD =
    R"foo({
    "$defs" : {
        "class_list" : ["Wizard", "Sorcerer", "Warlock", "Fighter", "Barbarian", "Artificer", "Rogue", "Monk", "Paladin"],
        "race_list" : ["Human", "Dwarf", "Elf", "Halfling"],
        "stat" : {
            "type" : "integer",
            "ui:step" : 1,
            "minimum" : 1,
            "maximum" : 20,
            "default" : 10
        },
        "character" : {
            "type" : "object",
            "ui:order" : ["name", "race", "class", "alignment_1", "alignment_2", "stats"],
            "properties" : {
                "name" : {"type" : "string"},
                "race" : {
                    "type" : "string",
                    "enum" : { "$ref" : "#/$defs/race_list" }
                },
                "class" : {
                    "type" : "string",
                    "enum" : { "$ref" : "#/$defs/class_list" }
                },
                "alignment_1" : {
                    "type" : "string",
                    "enum" : ["Lawful", "Neutral", "Chaotic"],
                    "ui:widget" : "button"
                },
                "alignment_2" : {
                    "type" : "string",
                    "enum" : ["Good", "Neutral", "Evil"],
                    "ui:widget" : "button"
                },
                "stats" : {
                    "type" : "object",
                    "properties" : {
                        "str" : { "$ref" : "#/$defs/stat" },
                        "con" : { "$ref" : "#/$defs/stat" },
                        "dex" : { "$ref" : "#/$defs/stat" },
                        "int" : { "$ref" : "#/$defs/stat" },
                        "wis" : { "$ref" : "#/$defs/stat" },
                        "cha" : { "$ref" : "#/$defs/stat" }
                    }
                }
            }
        }
    },
    "type": "object",
    "properties": {
        "party" : {
            "type" : "array",
            "items" : {
                "$ref" : "#/$defs/character"
            }
        }
    }
})foo";

constexpr auto PBR =
    R"foo({
    "$defs": {
        "normalized_number": {
            "type" : "number",
            "maximum": 1.0,
            "minimum": 0.0,
            "default": 0.0,
            "ui:widget" : "slider"
        }
    },
    "ui:order" : ["albedo", "metallic", "roughness", "emissive", "emissiveFactor", "unlit"],
    "properties" : {
        "metallic"  : { "$ref": "#/$defs/normalized_number" },
        "roughness" : { "$ref": "#/$defs/normalized_number" },
        "albedo"    : { "type" : "string", "ui:widget" : "color" },
        "emissive"  : { "type" : "string", "ui:widget" : "color" },
        "emissiveFactor" : {
            "type" : "number",
            "minimum" : 0.0,
            "ui:speed" : 0.01,
            "ui:widget" : "drag"
        },
        "unlit" : {
            "type" : "boolean"
        }
    },
    "type": "object"
})foo";


constexpr auto custom_widgets =
    R"foo({
    "description" : "See the source code example.cpp",
    "properties" : {
        "custom_number":
        {
            "type" : "number",
            "ui:widget" : "my_custom_number_widget"
        }
    },
    "type": "object"
})foo";

void runApp()
{

    static bool firstTime=true;
    if(firstTime)
    {
        firstTime=false;

        // lets define our own number widget by creating a lambda function
        // inside the widget map
        IJS::detail::widgets_all["number/my_custom_number_widget"] =
        [](char const* label, IJS::json & value, IJS::json const& _sch, IJS::json & cache, float object_width) -> bool
        {
            (void)_sch;
            (void)label;
            (void)object_width;
            auto W = ImGui::GetContentRegionAvail().x;

            // Use the "cache" object to store any temporary data
            // that may be used for drawing your widget
            float w = IJS::JValue(cache, "pos", 0.0f);
            w += 1.0f;
            if(w > W)
                w = 0;
            cache["pos"] = w;

            if( ImGui::Button(label, {w,0}) )
            {
                // when you set the value
                // make sure you return true
                value = value.get<float>() + 1.0f;
                return true;
            }

            return false;
        };
    }

    if(_schemaString.empty())
    {
        _schemaString = _schema.dump(4);
    }

    BeginFullScreen("Object");

    static bool _update = true;

    //=========================================================================
    // Examples
    //=========================================================================
    {
        if(ImGui::Button("Basic Number"))
        {
            _schemaString = basic_number;
            _update = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Basic Boolean"))
        {
            _schemaString = basic_boolean;
            _update = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Basic String"))
        {
            _schemaString = basic_string;
            _update = true;
        }

        ImGui::SameLine();

        if(ImGui::Button("Basic Object"))
        {
            _schemaString = basic_object;
            _update = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Basic Array"))
        {
            _schemaString = basic_array;
            _update = true;
        }

        if(ImGui::Button("Descriptions"))
        {
            _schemaString = descriptions;
            _update = true;
        }

        ImGui::SameLine();
        if(ImGui::Button("Ordering"))
        {
            _schemaString = ordering;
            _update = true;
        }

        ImGui::SameLine();

        if(ImGui::Button("Title"))
        {
            _schemaString = titles;
            _update = true;
        }

        ImGui::SameLine();

        if(ImGui::Button("Help"))
        {
            _schemaString = help;
            _update = true;
        }

        if(ImGui::Button("Number Widgets"))
        {
            _schemaString = number_widgets;
            _update = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Boolean Widgets"))
        {
            _schemaString = boolean_widgets;
            _update = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("String Widgets"))
        {
            _schemaString = string_widgets;
            _update = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Array Widgets"))
        {
            _schemaString = array_widgets;
            _update = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Object Widgets"))
        {
            _schemaString = object_widgets;
            _update = true;
        }

        if(ImGui::Button("Constants"))
        {
            _schemaString = constants;
            _update = true;
        }

        ImGui::SameLine();

        if(ImGui::Button("Enumerated Types"))
        {
            _schemaString = enumerated_types;
            _update = true;
        }


        if(ImGui::Button("$ref"))
        {
            _schemaString = references;
            _update = true;
        }

        ImGui::SameLine();
        if(ImGui::Button("D&D"))
        {
            _schemaString = DnD;
            _update = true;
        }

        ImGui::SameLine();
        if(ImGui::Button("PBR"))
        {
            _schemaString = PBR;
            _update = true;
        }
        if(_update)
        {
            _value.clear();
            _cache.clear();
        }

        if(ImGui::Button("Custom Widgets"))
        {
            _schemaString = custom_widgets;
            _update = true;
        }
        if(_update)
        {
            _value.clear();
            _cache.clear();
        }
    }


    auto width = ImGui::GetContentRegionAvail().x / 3;
    auto height= ImGui::GetContentRegionAvail().y;
    if(ImGui::BeginChild("Schema", {width, 0}))
    {
        ImGui::PushItemWidth(-1);
        if(ImGui::InputTextMultiline("##Schema", &_schemaString, {width, height}) || _update)
        {
            auto J = IJS::json::parse(_schemaString, nullptr, false);
            if(J.type() == IJS::json::value_t::discarded)
            {
                J = IJS::json::object_t();
                J["type"] = "object";
                J["description"] = "Error Parsing json";
            }

            IJS::jsonExpandAllReferences(J);
            _schema = std::move(J);
            _update = false;
        }
        ImGui::PopItemWidth();

        ImGui::EndChild();
    }

    ImGui::SameLine();

    static std::string _lastModifiedpath = "";
    if(ImGui::BeginChild("form", {width, 0}))
    {
        if(IJS::drawSchemaWidget("object",
                                  _value,
                                  _schema,
                                  _cache))
        {
            std::cout << "Last Edited JSON path: " << IJS::getModifiedWidgetPath() << std::endl;
            _lastModifiedpath = IJS::getModifiedWidgetPath();
        }
        ImGui::EndChild();
    }

    ImGui::SameLine();

    if(ImGui::BeginChild("Value", {width, 0}))
    {
        if(ImGui::CollapsingHeader("Last Modified path", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextUnformatted( _lastModifiedpath.c_str());
        }
        if(ImGui::CollapsingHeader("Output JSON", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextUnformatted(_value.dump(4).c_str());
        }
        if(ImGui::CollapsingHeader("Schema Cache", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextUnformatted(_cache.dump(4).c_str());
        }

        ImGui::EndChild();
    }


    ImGui::End();

    //ImGui::ShowDemoWindow(nullptr);
}

bool g_done = false;
SDL_WindowFlags window_flags = {};//static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
SDL_Window* window           = {};//SDL_CreateWindow("ImJSchema: Build ImGui Forms with JSON", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
SDL_Renderer* renderer       = {};//SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

void main_loop()
{
    ImGuiIO& io = ImGui::GetIO();
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            g_done = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
            g_done = true;
    }

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();


    runApp();

    // Rendering
    ImGui::Render();
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(clear_color.x * 255), static_cast<Uint8>(clear_color.y * 255), static_cast<Uint8>(clear_color.z * 255), static_cast<Uint8>(clear_color.w * 255));
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer);
}

bool initSDL()
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return false;
    }

// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with SDL_Renderer graphics context
    window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window     = SDL_CreateWindow("ImJSchema: Build ImGui Forms with JSON", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        SDL_Log("Error creating SDL_Renderer!");
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    return true;
}

void destroySDL()
{
    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int, char**)
{
    if(!initSDL())
    {
        return EXIT_FAILURE;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (!g_done)
    {
        main_loop();
    }
#endif

    destroySDL();

    return EXIT_SUCCESS;
}
