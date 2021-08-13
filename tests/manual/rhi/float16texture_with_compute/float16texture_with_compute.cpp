/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

// An advanced version of floattexture. Instead of RGBA32F, we use RGBA16F, and
// also generate the floating point data from rgba with compute. Then there's a
// compute pass using the BSDF prefiltering taken from Qt Quick 3D, which
// generates all the mip levels.

// Why do we animate the scale of the quad rendered to the window? To have
// different mip levels used, to prove that all of them are generated
// correctly, without artifacts (which would occur if memory barriers were not
// correctly generated by QRhi). For full verification use RenderDoc or similar.

#include "../shared/examplefw.h"
#include <qmath.h>

static float vertexData[] =
{ // Y up, CCW
  -0.5f,   0.5f,   0.0f, 0.0f,
  -0.5f,  -0.5f,   0.0f, 1.0f,
  0.5f,   -0.5f,   1.0f, 1.0f,
  0.5f,   0.5f,    1.0f, 0.0f
};

static quint16 indexData[] =
{
    0, 1, 2, 0, 2, 3
};

static const int MAX_MIP_LEVELS = 20;

struct {
    QVector<QRhiResource *> releasePool;

    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTexture *texRgba = nullptr;
    QRhiTexture *texFloat16 = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;

    QRhiBuffer *computeUBuf_load = nullptr;
    QRhiShaderResourceBindings *computeBindings_load = nullptr;
    QRhiComputePipeline *computePipeline_load = nullptr;
    QRhiBuffer *computeUBuf_prefilter = nullptr;
    QRhiShaderResourceBindings *computeBindings_prefilter[MAX_MIP_LEVELS];
    QRhiComputePipeline *computePipeline_prefilter = nullptr;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    bool computeDone = false;
    int mipCount;
    int prefilterUBufElemSize;
    quint32 prefilterNumWorkGroups[MAX_MIP_LEVELS][3];
    float scale = 2.5f;
    int scale_dir = -1;
} d;

void recordUploadThenFilterFloat16TextureWithCompute(QRhiCommandBuffer *cb)
{
    const int w = d.texRgba->pixelSize().width() / 16;
    const int h = d.texRgba->pixelSize().height() / 16;

    cb->beginComputePass();

    cb->setComputePipeline(d.computePipeline_load);
    cb->setShaderResources();
    cb->dispatch(w, h, 1);

    cb->setComputePipeline(d.computePipeline_prefilter);
    for (int level = 1; level < d.mipCount; ++level) {
        const int i = level - 1;
        const int mipW = d.prefilterNumWorkGroups[i][0];
        const int mipH = d.prefilterNumWorkGroups[i][1];
        QPair<int, quint32> dynamicOffset = { 0, quint32(d.prefilterUBufElemSize * i) };
        cb->setShaderResources(d.computeBindings_prefilter[i], 1, &dynamicOffset);
        cb->dispatch(mipW, mipH, 1);
    }

    cb->endComputePass();
}

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::Compute))
        qFatal("Compute is not supported");

    if (!m_r->isTextureFormatSupported(QRhiTexture::RGBA16F))
        qFatal("RGBA16F texture format is not supported");

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    // load rgba8 image data

    QImage image;
    image.load(QLatin1String(":/qt256.png"));
    image = image.convertToFormat(QImage::Format_RGBA8888);
    Q_ASSERT(!image.isNull());
    d.texRgba = m_r->newTexture(QRhiTexture::RGBA8, image.size(), 1, QRhiTexture::UsedWithLoadStore);
    d.texRgba->build();
    d.releasePool << d.texRgba;

    d.initialUpdates->uploadTexture(d.texRgba, image);

    d.mipCount = m_r->mipLevelsForSize(image.size());
    Q_ASSERT(d.mipCount <= MAX_MIP_LEVELS);

    d.texFloat16 = m_r->newTexture(QRhiTexture::RGBA16F, image.size(), 1, QRhiTexture::UsedWithLoadStore | QRhiTexture::MipMapped);
    d.releasePool << d.texFloat16;
    d.texFloat16->build();

    // compute

    d.computeUBuf_load = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 12);
    d.computeUBuf_load->build();
    d.releasePool << d.computeUBuf_load;

    quint32 numWorkGroups[3] = { quint32(image.width()), quint32(image.height()), 0 };
    d.initialUpdates->updateDynamicBuffer(d.computeUBuf_load, 0, 12, numWorkGroups);

    d.computeBindings_load = m_r->newShaderResourceBindings();
    d.computeBindings_load->setBindings({
                                            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::ComputeStage, d.computeUBuf_load),
                                            QRhiShaderResourceBinding::imageLoad(1, QRhiShaderResourceBinding::ComputeStage, d.texRgba, 0),
                                            QRhiShaderResourceBinding::imageStore(2, QRhiShaderResourceBinding::ComputeStage, d.texFloat16, 0)
                                        });
    d.computeBindings_load->build();
    d.releasePool << d.computeBindings_load;

    d.computePipeline_load = m_r->newComputePipeline();
    d.computePipeline_load->setShaderResourceBindings(d.computeBindings_load);
    d.computePipeline_load->setShaderStage({ QRhiShaderStage::Compute, getShader(QLatin1String(":/load.comp.qsb")) });
    d.computePipeline_load->build();
    d.releasePool << d.computePipeline_load;

    d.prefilterUBufElemSize = m_r->ubufAligned(12);
    d.computeUBuf_prefilter = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, d.prefilterUBufElemSize * d.mipCount);
    d.computeUBuf_prefilter->build();
    d.releasePool << d.computeUBuf_prefilter;

    int mipW = image.width() >> 1;
    int mipH = image.height() >> 1;
    for (int level = 1; level < d.mipCount; ++level) {
        const int i = level - 1;
        d.prefilterNumWorkGroups[i][0] = quint32(mipW);
        d.prefilterNumWorkGroups[i][1] = quint32(mipH);
        d.prefilterNumWorkGroups[i][2] = 0;
        d.initialUpdates->updateDynamicBuffer(d.computeUBuf_prefilter, d.prefilterUBufElemSize * i, 12, d.prefilterNumWorkGroups[i]);
        mipW = mipW > 2 ? mipW >> 1 : 1;
        mipH = mipH > 2 ? mipH >> 1 : 1;

        d.computeBindings_prefilter[i] = m_r->newShaderResourceBindings();
        d.computeBindings_prefilter[i]->setBindings({
                                                        QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::ComputeStage, d.computeUBuf_prefilter, 12),
                                                        QRhiShaderResourceBinding::imageLoad(1, QRhiShaderResourceBinding::ComputeStage, d.texFloat16, level - 1),
                                                        QRhiShaderResourceBinding::imageStore(2, QRhiShaderResourceBinding::ComputeStage, d.texFloat16, level)
                                                    });
        d.computeBindings_prefilter[i]->build();
        d.releasePool << d.computeBindings_prefilter[i];
    }

    d.computePipeline_prefilter = m_r->newComputePipeline();
    d.computePipeline_prefilter->setShaderResourceBindings(d.computeBindings_prefilter[0]); // just need a layout compatible one
    d.computePipeline_prefilter->setShaderStage({ QRhiShaderStage::Compute, getShader(QLatin1String(":/prefilter.comp.qsb")) });
    d.computePipeline_prefilter->build();
    d.releasePool << d.computePipeline_prefilter;

    // graphics

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
    d.vbuf->build();
    d.releasePool << d.vbuf;

    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indexData));
    d.ibuf->build();
    d.releasePool << d.ibuf;

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    d.ubuf->build();
    d.releasePool << d.ubuf;

    // enable mipmaps
    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->build();

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.texFloat16, d.sampler)
    });
    d.srb->build();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;
    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/texture.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 4 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->build();

    d.initialUpdates->uploadStaticBuffer(d.vbuf, vertexData);
    d.initialUpdates->uploadStaticBuffer(d.ibuf, indexData);

    qint32 flip = 0;
    d.initialUpdates->updateDynamicBuffer(d.ubuf, 64, 4, &flip);
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    QMatrix4x4 mvp = m_proj;
    mvp.scale(d.scale);
    d.scale += d.scale_dir * 0.01f;
    if (qFuzzyIsNull(d.scale) || d.scale >= 2.5f)
        d.scale_dir *= -1;
    u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());

    cb->resourceUpdate(u);

    // If not yet done, then do a compute pass that uploads level 0, doing an
    // rgba8 -> float16 conversion. Follow that with another compute pass to do
    // the filtering and generate all the mip levels.
    if (!d.computeDone) {
        recordUploadThenFilterFloat16TextureWithCompute(cb);
        d.computeDone = true;
    }

    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(6);
    cb->endPass();
}
