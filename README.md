# How this font seemingly works

The font system is complicated. 
The .fnt file contains a list of characters and their attributes. That structure is 0x18 bytes long.\
![image](https://user-images.githubusercontent.com/69110695/173181150-c4cbe3c0-29fa-4ea1-b933-d82b8e8d2fdc.png)\
The first 4 bytes are the unicode of the character (here 0x20, space); \
the following 4 bytes are an integer which we will call Type, which can be 0 1 2 4 in the base font, but could actually be 3, 5, 6, 7 as well (which would be a mix of 1/2/4 together, I didn't think about the implications yet.)\
The two shorts after are the origin X and Y in the GNF file in pixels. \
The two shorts after are the width (0xC) and height (0xE).\
The short after is either 0x100 or 0x200 (maybe 0x300 works too, no idea), 0x100 is green, 0x200 is red, and theyre used to separate green and red characters with bit masking.\
The short after (0x12) is used in some kerning calculations although its use is not clear.\
The short after (0x14) is used in the Y offset, use is clear and constant between contexts (always used the same way), computed according to a baseline\
The final short (0x16) is technically the distance to the next character but only in the context of the info box in battle (for some reason).\
Setting it to bitmap->width + 2 makes it work (only in info box) pretty neatly.

Now a look at the code parsing the fnt file:
![image](https://user-images.githubusercontent.com/69110695/173181046-cd594531-b197-4b6b-841b-034e143b56f2.png)
First the unicode is computed based on the UTF8 in tbl/scripts/exe\
Then that unicode is looked for in the fnt file; if it doesn't exist, it will be replaced by "?"\
If it does exist (the case we are interested in):\
The context is first retrieved. I'd say the infobox might be context = 2 but I'm not sure; since this one works fine, I'm focusing on the other contexts.\
For the other contexts, first a constant (probably from the exe but tricky to track tbh) is obtained from (DAT_01801a80 + 0x5C8) (we can guess DAT_01801a80 is a font related structure or something.\
If the Type is 1, that constant is divided by 2. At this point I'm reminded of the font width which used a kerning method like this in Sky FC, where the characters in range 0xA1 - 0xCF would use a hardcoded half current width kerning. Anyway, this is not something that we can change unless exe changes which I don't want to make.\
Then the 0x12 short is obtained from the very first character in the font list (the space character).\ 
The following equation:


Something = (pointer_character + 0xc) + space0x12 * -2;\
is hard to explain.

(pointer_character + 0xc) is the width of the bitmap, so if we suppose the 0x12 byte of space is set to 0, that something will equal the bitmap width.\
if this bitmap width is lesser than the constant in exe, that something will become the constant in exe (which isn't good news at all let's be honest)\
Finally that something is converted in float, and the result will be called "first kerning".

Now after the first kerning is computed, a function is called which calculates something else based on the context and the Type as well, following a similar logic.

![image](https://user-images.githubusercontent.com/69110695/173181645-eac917e3-145e-4a3f-aaa9-191bdaff1844.png)

That logic is as follows: \
first the bitmap width is obtained and divided by 2.\
If the context is battle info, it will only add the 0x12 byte to the halfwidth.\
In other contexts, for Type 2, the halfwidth will be returned directly\
for Type = 1,\
the constant in exe is retrieved and divided by 2, then the exact same equation as before appears except the result is divided by 2 and added to the halfwidth before being returned in XMM0.\
For Type 4, Same as Type 1 except we remove the bitmap width to it and add that result to half width. 

Type 4 has been observed as leaving a huge space left to the character being change.

Finally, outside of this function, we have this block: \
![image](https://user-images.githubusercontent.com/69110695/173181958-c81ac857-8474-446d-b45f-39b4cba48db3.png)
Which means that the first kerning is multiplied by 1.0 before being added to the second kerning, then another value is multiplying the total.

This means the kerning, or actually the position of the character, is seemingly controlled by two components, the first and the second "kerning".


Additional note: I realized whether or not the 0x16 short is set to bitmap->width + 2 it will have no influence on the dialog kerning.
The dialog kerning when Type is set to 1 seems to work like this: They take half of the bitmap of the character and add a constant offset of "current font width" / 2, the next character will be drawn with the center of its bitmap at this position. This means in "time", the m will be closer to the i than in "s." where the width of the dot character is so tiny that it leaves a lot of space on the left.

Another note:
The constant hardcoded in exe seems to be 0x2C (44) pixels.
![image](https://user-images.githubusercontent.com/69110695/173191788-6d100277-b882-4a28-912a-1124e36c4a33.png)
\in Type 0, that gap of 44 will be present and you can control it to improve it a bit using the 0x12 byte of the space character (don't ask me how the f it works like this but the code suggests this), but you CAN'T reduce it as it will get saturated to 44 if you get below it.\
in Type 1, that gap becomes 22. You still can't reduce it, only add to it.\
in Type 2, I think that gap is present and the 0x16 byte will add to it in the right.\
finally in Type 4, while that gap is present the 0x16 byte will to it to the left\
Note: If we set a negative gap in Type 2, it will fuck up the battle info box which relies on 0x16 without the gap.


