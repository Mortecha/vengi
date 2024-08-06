/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/List.h"
#include "core/NonCopyable.h"
#include "core/collection/StringMap.h"
#include <stdint.h>
#include <string.h>
#include <glm/fwd.hpp>
#include "ShaderTypes.h"

namespace video {

class UniformBuffer;

#ifndef VERTEX_POSTFIX
#define VERTEX_POSTFIX ".vert"
#endif

#ifndef FRAGMENT_POSTFIX
#define FRAGMENT_POSTFIX ".frag"
#endif

#ifndef GEOMETRY_POSTFIX
#define GEOMETRY_POSTFIX ".geom"
#endif

#ifndef COMPUTE_POSTFIX
#define COMPUTE_POSTFIX ".comp"
#endif

// activate this to validate that every uniform was set
#define VALIDATE_UNIFORMS 0

/**
 * @brief Shader wrapper for GLSL. See shadertool for autogenerated shader wrapper code
 * from vertex and fragment shaders
 * @ingroup Video
 */
class Shader : public core::NonCopyable {
protected:
	typedef core::Array<Id, (int)ShaderType::Max> ShaderArray;
	ShaderArray _shader;

	typedef core::Map<int, uint64_t, 8> UniformStateMap;
	mutable UniformStateMap _uniformStateMap{128};

	Id _program = InvalidId;
	bool _initialized = false;
	mutable bool _active = false;
	bool _dirty = true;

	typedef core::StringMap<core::String> ShaderDefines;
	ShaderDefines _defines{128};

	typedef core::StringMap<int> ShaderUniformArraySizes;
	ShaderUniformArraySizes _uniformArraySizes{128};

	ShaderUniforms _uniforms;

	// can be used to validate that every uniform was set. The value type is the location index
	mutable core::Map<int, bool, 4> _usedUniforms{128};
	bool _recordUsedUniforms = false;
	void addUsedUniform(int location) const;

	ShaderAttributes _attributes{128};

	typedef core::Map<int, int> AttributeComponents;
	AttributeComponents _attributeComponents{128};

	mutable uint32_t _time = 0u;

	core::String _name;

	const Uniform* getUniform(const core::String& name) const;

	int fetchUniforms();

	int fetchAttributes();

	bool createProgramFromShaders();

	/**
	 * @param[in] location The uniform location in the shader
	 * @param[in] value The buffer with the data
	 * @param[in] length The length in bytes of the given value buffer
	 * @return @c false if no change is needed, @c true if we have to update the value
	 */
	bool checkUniformCache(int location, const void* value, int length) const;

public:
	Shader();
	virtual ~Shader();

	/**
	 * Some drivers don't support underscores in their defines...
	 */
	static core::String validPreprocessorName(const core::String& name);

	static int glslVersion;

	virtual void shutdown();

	bool load(const core::String& name, const core::String& buffer, ShaderType shaderType);

	core::String getSource(ShaderType shaderType, const core::String& buffer, bool finalize = true, core::List<core::String>* includedFiles = nullptr) const;

	Id handle() const;

	/**
	 * If the shaders were loaded manually via @c ::load, then you have to initialize the shader manually, too
	 */
	bool init();

	bool isInitialized() const;

	/**
	 * @brief The dirty state can be used to determine whether you have to set some
	 * uniforms again because the shader was reinitialized. This must be used manually
	 * if you set e.g. uniforms only once after init
	 * @sa isDirty()
	 */
	void markClean();
	/**
	 * @sa markClean()
	 */
	void markDirty();
	/**
	 * @sa markClean()
	 */
	bool isDirty() const;

	/**
	 * @brief Make sure to configure feedback transform varying before you link the shader
	 * @see setupTransformFeedback()
	 */
	virtual bool setup() {
		return false;
	}

	void recordUsedUniforms(bool state);

	void clearUsedUniforms();

	bool loadFromFile(const core::String& filename, ShaderType shaderType);

	/**
	 * @brief Loads a vertex and fragment shader for the given base filename.
	 *
	 * The filename is hand over to your @c Context implementation with the appropriate filename postfixes
	 *
	 * @see VERTEX_POSTFIX
	 * @see FRAGMENT_POSTFIX
	 */
	bool loadProgram(const core::String& filename);
	bool reload();

	/**
	 * @brief Returns the raw shader handle
	 */
	Id getShader(ShaderType shaderType) const;

	/**
	 * @brief Ticks the shader
	 */
	virtual void update(uint32_t deltaTime);

	/**
	 * @brief Bind the shader program
	 *
	 * @return @c true if is is useable now, @c false if not
	 */
	virtual bool activate() const;

	virtual bool deactivate() const;

	bool isActive() const;

	/**
	 * @brief Run the compute shader.
	 * @return @c false if this is no compute shader, or the execution failed.
	 */
	bool run(const glm::uvec3& workGroups, bool wait = false);

	void checkAttribute(const core::String& attribute);
	void checkUniform(const core::String& uniform);
	void checkAttributes(std::initializer_list<core::String> attributes);
	void checkUniforms(std::initializer_list<core::String> uniforms);

	/**
	 * @brief Adds a new define in the form '#define value' to the shader source code
	 */
	void addDefine(const core::String& name, const core::String& value);

	void setUniformArraySize(const core::String& name, int size);
	void setAttributeComponents(int location, int size);
	int getAttributeComponents(int location) const;
	int getAttributeComponents(const core::String& name) const;

	/**
	 * @return -1 if uniform wasn't found, or no size is known. If the uniform is known, but
	 * it is no array, this will return 0
	 */
	int getUniformArraySize(const core::String& name) const;

	int checkAttributeLocation(const core::String& name) const;
	int getAttributeLocation(const core::String& name) const;
	bool setAttributeLocation(const core::String& name, int location);

	int getUniformLocation(const core::String& name) const;

	void setUniform(int location, TextureUnit value) const;
	void setVertexAttribute(const core::String& name, int size, DataType type, bool normalize, int stride, const void* buffer) const;
	void setVertexAttributeInt(const core::String& name, int size, DataType type, int stride, const void* buffer) const;
	void disableVertexAttribute(const core::String& name) const;
	int enableVertexAttributeArray(const core::String& name) const;
	bool hasAttribute(const core::String& name) const;
	bool hasUniform(const core::String& name) const;
	bool isUniformBlock(const core::String& name) const;

	// particular renderer api must implement this

private:
	// only called from setting the texture units
	void setUniformi(int location, int value) const;
public:
	int32_t getUniformBufferOffset(const char *name);
	bool setUniformBuffer(const core::String& name, const UniformBuffer& buffer);
	void setVertexAttribute(int location, int size, DataType type, bool normalize, int stride, const void* buffer) const;
	void setVertexAttributeInt(int location, int size, DataType type, int stride, const void* buffer) const;
	void disableVertexAttribute(int location) const;
	bool enableVertexAttributeArray(int location) const;
	/**
	 * In instanced rendering, you draw multiple instances (copies) of the same object with a single command. However,
	 * you often want each instance to have some unique attributes, like different positions, colors, or sizes. This is
	 * where @c setDivisor comes into play.
	 *
	 * This sets a divisor for an attribute. The divisor determines how often the attribute changes when drawing
	 * multiple instances.
	 *
	 * Example:
	 * @li If you set the divisor to 0 (default), the attribute changes with each vertex. This is normal for vertex
	 * attributes.
	 * @li If you set the divisor to 1, the attribute changes with each instance, not each vertex. This means every
	 * instance gets a unique value of that attribute.
	 */
	bool setDivisor(int location, uint32_t divisor) const;
};

inline bool Shader::isDirty() const {
	return _dirty;
}

inline bool Shader::isInitialized() const {
	return _initialized;
}

inline void Shader::addUsedUniform(int location) const {
	_usedUniforms.put(location, true);
}

inline void Shader::recordUsedUniforms(bool state) {
	_recordUsedUniforms = state;
}

inline void Shader::clearUsedUniforms() {
	_usedUniforms.clear();
}

inline void Shader::setUniform(int location, TextureUnit value) const {
	setUniformi(location, core::enumVal(value));
}

inline Id Shader::handle() const {
	return _program;
}

/**
 * @brief Activates the given given and disables it again (if it wasn't active before)
 * if the scope is left
 */
class ScopedShader {
private:
	const Shader& _shader;
	const Id _oldShader;
	bool _alreadyActive;
public:
	ScopedShader(const Shader& shader);
	~ScopedShader();
};

#define shaderSetUniformIf(shader, func, var, ...) if (shader.hasUniform(var)) { shader.func(var, __VA_ARGS__); }

}
