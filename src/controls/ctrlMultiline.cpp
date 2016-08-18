// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "defines.h" // IWYU pragma: keep
#include "ctrlMultiline.h"
#include "ctrlScrollBar.h"
#include "ogl/glArchivItem_Font.h"
#include "driver/src/MouseCoords.h"
#include "CollisionDetection.h"
#include <boost/foreach.hpp>
#include <algorithm>

ctrlMultiline::ctrlMultiline(Window* parent, unsigned int id,
                             const DrawPoint& pos, unsigned short width, unsigned short height,
                             TextureColor tc, glArchivItem_Font* font, unsigned format):
    Window(pos, id, parent, width, height),
    tc_(tc), font(font), format_(format), showBackground_(true)
{
    RecalcVisibleLines();
    AddScrollBar(0, width - SCROLLBAR_WIDTH, 0, SCROLLBAR_WIDTH, height, SCROLLBAR_WIDTH, tc, maxNumVisibleLines);
}

/**
 *  fügt eine Zeile hinzu.
 */
void ctrlMultiline::AddString(const std::string& str, unsigned int color, bool scroll)
{
    lines.push_back(Line(str, color));
    RecalcWrappedLines();

    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);
    if (scroll && scrollbar->GetPos() + 1 + maxNumVisibleLines == lines.size())
        scrollbar->SetPos(scrollbar->GetPos() + 1);
}

void ctrlMultiline::Clear()
{
    lines.clear();
    RecalcWrappedLines();
}

/**
 *  zeichnet das Fenster.
 */
bool ctrlMultiline::Draw_()
{
    if(showBackground_)
        Draw3D(GetDrawPos(), width_, height_, tc_, 2);

    DrawControls();

    unsigned numVisibleLines = std::min<unsigned>(maxNumVisibleLines, drawLines.size());

    unsigned scrollbarPos = GetCtrl<ctrlScrollBar>(0)->GetPos();
    DrawPoint curPos = GetDrawPos() + DrawPoint(PADDING, PADDING);
    for(unsigned i = 0; i < numVisibleLines; ++i)
    {
        font->Draw(curPos, drawLines[i + scrollbarPos].str, format_, drawLines[i + scrollbarPos].color);
        curPos.y += font->getHeight();
    }

    return true;
}

void ctrlMultiline::RecalcVisibleLines()
{
    if(GetHeight() < 2 * PADDING)
        maxNumVisibleLines = 0;
    else
        maxNumVisibleLines = (GetHeight() - 2 * PADDING) / font->getHeight();
}

void ctrlMultiline::RecalcWrappedLines()
{
    drawLines.clear();
    // No space for a single line, or to narrow to even show the scrollbar -> Bail out
    if(maxNumVisibleLines == 0 || width_ < 2 * PADDING + SCROLLBAR_WIDTH)
    {
        GetCtrl<ctrlScrollBar>(0)->SetRange(0);
        return;
    }
    // Calculate the wrap info for each real line (2nd pass if we need a scrollbar after breaking into lines)
    std::vector<glArchivItem_Font::WrapInfo> wrapInfos;
    wrapInfos.reserve(lines.size());
    bool needScrollBar = lines.size() > maxNumVisibleLines && scrollbarAllowed_;
    do{
        wrapInfos.clear();
        unsigned curNumLines = 0;
        unsigned maxTextWidth = width_ - 2 * PADDING;
        if(needScrollBar)
            maxTextWidth -= SCROLLBAR_WIDTH;
        for(unsigned i = 0; i < lines.size(); i++)
        {
            wrapInfos.push_back(font->GetWrapInfo(lines[i].str, maxTextWidth, maxTextWidth));
            if(!needScrollBar)
            {
                curNumLines += wrapInfos[i].positions.size();
                if(curNumLines > maxNumVisibleLines)
                    break;
            }
        }
        // We are done, if we already knew we need a scrollbar (latest at 2nd pass)
        // or if we don't need it even after potentially breaking long lines
        if(needScrollBar || !scrollbarAllowed_ || curNumLines <= maxNumVisibleLines)
            break;
        else
            needScrollBar = true;
    } while(true); // Endless loop, exited at latest after 2nd pass

    // New create the actually drawn lines
    for(unsigned i = 0; i < wrapInfos.size(); i++)
    {
        // Special case: No break, just push the line as-is
        if(wrapInfos[i].positions.size() == 1u)
            drawLines.push_back(lines[i]);
        else
        {
            // Break it
            const std::vector<std::string> newLines = wrapInfos[i].CreateSingleStrings(lines[i].str);
            BOOST_FOREACH(const std::string& line, newLines)
                drawLines.push_back(Line(line, lines[i].color));
        }
    }
    // If we don't have a scrollbar, restrict to maximum displayable lines
    if(!scrollbarAllowed_ && drawLines.size() > maxNumVisibleLines)
        drawLines.resize(maxNumVisibleLines);
    GetCtrl<ctrlScrollBar>(0)->SetRange(drawLines.size());
}

bool ctrlMultiline::Msg_LeftDown(const MouseCoords& mc)
{
    return GetCtrl<Window>(0)->Msg_LeftDown(mc);
}

bool ctrlMultiline::Msg_LeftUp(const MouseCoords& mc)
{
    return GetCtrl<Window>(0)->Msg_LeftUp(mc);
}
bool ctrlMultiline::Msg_WheelUp(const MouseCoords& mc)
{
    if(Coll(mc.x, mc.y, GetX() + PADDING, GetY() + PADDING, width_ - 2 * PADDING, height_ - 2 * PADDING))
    {
        ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(-3);
        return true;
    }
    else
        return false;
}


bool ctrlMultiline::Msg_WheelDown(const MouseCoords& mc)
{
    if(Coll(mc.x, mc.y, GetX() + PADDING, GetY() + PADDING, width_ - 2 * PADDING, height_ - 2 * PADDING))
    {
        ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(+3);
        return true;
    }
    else
        return false;
}

bool ctrlMultiline::Msg_MouseMove(const MouseCoords& mc)
{
    return GetCtrl<Window>(0)->Msg_MouseMove(mc);
}

void ctrlMultiline::Resize(unsigned short width, unsigned short height)
{
    Point<unsigned short> oldSize(GetWidth(), GetHeight());
    Window::Resize(width, height);

    RecalcVisibleLines();
    GetCtrl<ctrlScrollBar>(0)->SetPageSize(maxNumVisibleLines);
    GetCtrl<ctrlScrollBar>(0)->Move(width - SCROLLBAR_WIDTH, 0);
    // Do only if vertical space changed or horizontal space decreased (might need scrollbar then)
    if(GetWidth() != oldSize.x || GetHeight() < oldSize.y)
        RecalcWrappedLines();
}

/// Textzeile ersetzen. Klappt bestimmt nicht mit Scrollbar-Kram
void ctrlMultiline::SetLine(const unsigned index, const std::string& str, unsigned int color)
{
    if (index < lines.size())
    {
        lines[index] = Line(str, color);
        RecalcWrappedLines();
    }
}

void ctrlMultiline::SetNumVisibleLines(unsigned numLines)
{
    SetHeight(numLines * font->getHeight() + 2 * PADDING);
}

unsigned ctrlMultiline::GetContentHeight() const
{
    return std::min<unsigned>(GetHeight(), drawLines.size() * font->getHeight() + 2 * PADDING);
}

unsigned ctrlMultiline::GetContentWidth() const
{
    unsigned maxWidth = 0;
    unsigned addWidth = 2 * PADDING;
    if(drawLines.size() > maxNumVisibleLines)
        addWidth += SCROLLBAR_WIDTH;
    BOOST_FOREACH(const Line& line, drawLines)
    {
        unsigned curWidth = font->getWidth(line.str) + addWidth;
        if(curWidth > maxWidth)
        {
            maxWidth = curWidth;
            if(maxWidth >= GetWidth())
                return GetWidth();
        }
    }
    return std::min<unsigned>(GetWidth(), maxWidth);
}
