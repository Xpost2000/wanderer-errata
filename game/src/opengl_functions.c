void init_gl_functions( void ){
    LOAD_GL( PFNGLGENBUFFERS, glGenBuffers );
    LOAD_GL( PFNGLBINDBUFFER, glBindBuffer );
    LOAD_GL( PFNGLBUFFERDATA, glBufferData );
    LOAD_GL( PFNGLDELETEBUFFERS, glDeleteBuffers );
    LOAD_GL( PFNGLGENVERTEXARRAYS, glGenVertexArrays );
    LOAD_GL( PFNGLDELETEVERTEXARRAYS, glDeleteVertexArrays );
    LOAD_GL( PFNGLBINDVERTEXARRAY, glBindVertexArray );
    LOAD_GL( PFNGLVERTEXATTRIBPOINTER, glVertexAttribPointer );
    LOAD_GL( PFNGLENABLEVERTEXATTRIBARRAY, glEnableVertexAttribArray );
    LOAD_GL( PFNGLCREATEPROGRAM ,glCreateProgram );
    LOAD_GL( PFNGLDELETEPROGRAM ,glDeleteProgram );
    LOAD_GL( PFNGLCREATESHADER ,glCreateShader );
    LOAD_GL( PFNGLDELETESHADER ,glDeleteShader );
    LOAD_GL( PFNGLSHADERSOURCE ,glShaderSource );
    LOAD_GL( PFNGLATTACHSHADER ,glAttachShader );
    LOAD_GL( PFNGLDETACHSHADER ,glDetachShader );
    LOAD_GL( PFNGLUSEPROGRAM ,glUseProgram );
    LOAD_GL( PFNGLCOMPILESHADER ,glCompileShader );
    LOAD_GL( PFNGLLINKPROGRAM ,glLinkProgram );
    LOAD_GL( PFNGLGETUNIFORMLOCATION ,glGetUniformLocation );
    LOAD_GL( PFNGLUNIFORM1F ,glUniform1f );
    LOAD_GL( PFNGLUNIFORM2F ,glUniform2f );
    LOAD_GL( PFNGLUNIFORM3F ,glUniform3f );
    LOAD_GL( PFNGLUNIFORM1I ,glUniform1i );
    LOAD_GL( PFNGLUNIFORM2I ,glUniform2i );
    LOAD_GL( PFNGLUNIFORM3I ,glUniform3i );
    LOAD_GL( PFNGLUNIFORMMATRIX ,glUniformMatrix4fv );
    LOAD_GL( PFNGLGETPROGRAMIV, glGetProgramiv );
    LOAD_GL( PFNGLGETPROGRAMINFOLOG, glGetProgramInfoLog );
    LOAD_GL( PFNGLGETSHADERIV, glGetShaderiv );
    LOAD_GL( PFNGLGETSHADERINFOLOG, glGetShaderInfoLog);

    LOAD_GL( PFNGLGENERATEMIPMAP, glGenerateMipmap );
    // NOTE(jerry): pls check?
#ifdef __WIN32
    LOAD_GL( PFNGLACTIVETEXTURE, glActiveTexture );
#endif
}
