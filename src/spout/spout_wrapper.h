#pragma once

// Spout wrapper — C interface to avoid GL header conflicts with Geode PCH.
// This header is safe to include from Geode-compiled code.

#ifdef __cplusplus
extern "C" {
#endif

/// Initialize the Spout sender with the given name.
/// Returns true on success.
int spout_init(const char* senderName);

/// Send an OpenGL FBO to Spout.
/// fboId: the OpenGL framebuffer object ID (0 = default).
/// texId: the OpenGL texture ID bound to the FBO.
/// width, height: dimensions of the texture.
/// Returns true on success.
int spout_send_texture(unsigned int texId, unsigned int width, unsigned int height);

/// Send raw RGBA pixel data to Spout.
int spout_send_image(const unsigned char* pixels, unsigned int width, unsigned int height);

/// Release the Spout sender and free resources.
void spout_release();

/// Check if sender is initialized.
int spout_is_initialized();

#ifdef __cplusplus
}
#endif
