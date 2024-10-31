#pragma once

#include "gdkd3d12texture.h"

#include "gdkd3d12texturebuilder.h"

G_BEGIN_DECLS

GdkTexture *            gdk_d3d12_texture_new_from_builder  (GdkD3d12TextureBuilder *builder,
                                                             GDestroyNotify           destroy,
                                                             gpointer                 data,
                                                             GError                 **error);

HANDLE                  gdk_d3d12_texture_get_handle        (GdkD3d12Texture         *self);

guint                   gdk_d3d12_texture_import_gl         (GdkD3d12Texture         *self,
                                                             GdkGLContext            *context,
                                                             guint                   *out_mem_id);

G_END_DECLS

