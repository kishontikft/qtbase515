//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_DIRECTIVEHANDLER_H_
#define COMPILER_TRANSLATOR_DIRECTIVEHANDLER_H_

#include "common/angleutils.h"
#include "compiler/translator/ExtensionBehavior.h"
#include "compiler/translator/Pragma.h"
#include "compiler/preprocessor/DirectiveHandlerBase.h"
#include "GLSLANG/ShaderLang.h"

namespace sh
{
class TDiagnostics;

class TDirectiveHandler : public pp::DirectiveHandler, angle::NonCopyable
{
  public:
    TDirectiveHandler(TExtensionBehavior &extBehavior,
                      TDiagnostics &diagnostics,
                      int &shaderVersion,
                      sh::GLenum shaderType,
                      bool debugShaderPrecisionSupported);
    ~TDirectiveHandler() override;

    const TPragma &pragma() const { return mPragma; }
    const TExtensionBehavior &extensionBehavior() const { return mExtensionBehavior; }

    void handleError(const pp::SourceLocation &loc, const std::string &msg) override;

    void handlePragma(const pp::SourceLocation &loc,
                      const std::string &name,
                      const std::string &value,
                      bool stdgl) override;

    void handleExtension(const pp::SourceLocation &loc,
                         const std::string &name,
                         const std::string &behavior) override;

    void handleVersion(const pp::SourceLocation &loc, int version) override;

  private:
    TPragma mPragma;
    TExtensionBehavior &mExtensionBehavior;
    TDiagnostics &mDiagnostics;
    int &mShaderVersion;
    sh::GLenum mShaderType;
    bool mDebugShaderPrecisionSupported;
};

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_DIRECTIVEHANDLER_H_
