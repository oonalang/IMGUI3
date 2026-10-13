/* Minimal module list for the Android menu build: TTF/OTF rasterization only.
   Written for static linking with the sources vendored under jni/ImGui/freetype/freetype_ttf_src. */
FT_USE_MODULE( FT_Module_Class, sfnt_module_class )
FT_USE_MODULE( FT_Module_Class, tt_driver_class )
FT_USE_MODULE( FT_Module_Class, cff_driver_class )
FT_USE_MODULE( FT_Renderer_Class, ft_smooth_renderer_class )
FT_USE_MODULE( FT_Module_Class, psnames_module_class )
FT_USE_MODULE( FT_Module_Class, psaux_module_class )
FT_USE_MODULE( FT_Module_Class, pshinter_module_class )
