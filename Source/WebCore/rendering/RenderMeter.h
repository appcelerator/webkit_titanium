/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RenderMeter_h
#define RenderMeter_h

#if ENABLE(METER_TAG)
#include "RenderBlock.h"
#include "RenderIndicator.h"
#include "RenderWidget.h"


namespace WebCore {

class HTMLMeterElement;
class ShadowBlockElement;

class RenderMeter : public RenderIndicator {
public:
    RenderMeter(HTMLMeterElement*);
    virtual ~RenderMeter();

private:
    virtual const char* renderName() const { return "RenderMeter"; }
    virtual bool isMeter() const { return true; }
    virtual void updateFromElement();
    virtual void computeLogicalWidth();
    virtual void computeLogicalHeight();

    virtual void layoutParts();
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    void updateShadows();
    void detachShadows();

    bool shadowAttached() const { return m_barPart; }
    IntRect valuePartRect() const;
    PseudoId valuePseudoId() const;
    IntRect barPartRect() const;
    PseudoId barPseudoId() const;

    double valueRatio() const;
    bool shouldHaveParts() const;
    PassRefPtr<ShadowBlockElement> createPart(PseudoId);

    RefPtr<ShadowBlockElement> m_barPart;
    RefPtr<ShadowBlockElement> m_valuePart;
};

inline RenderMeter* toRenderMeter(RenderObject* object)
{
    ASSERT(!object || object->isMeter());
    return static_cast<RenderMeter*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderMeter(const RenderMeter*);

} // namespace WebCore

#endif

#endif // RenderMeter_h

