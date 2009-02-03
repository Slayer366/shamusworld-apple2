//
// Static text class
//
// by James L. Hammons
//

#ifndef __TEXT_H__
#define __TEXT_H__

#include <string>
#include "element.h"

class Text: public Element
{
	public:
		Text(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0, Element * parent = NULL);
		Text(uint32 x, uint32 y, std::string s, uint32 fg = 0xFF8484FF, uint32 bg = 0xFF84FF4D, Element * parent = NULL);
		virtual void HandleKey(SDLKey key) {}
		virtual void HandleMouseMove(uint32 x, uint32 y) {}
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown) {}
		virtual void Draw(void);
		virtual void Notify(Element *) {}
		void SetText(std::string s);

	protected:
//		uint32 fgColor, bgColor;
		std::string text;
};

#endif	// __TEXT_H__
