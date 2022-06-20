# Preview of the current result
Translation in french of a scene from Kuro no Kiseki using a font generated with this tool
https://www.youtube.com/watch?v=siPvwii_7YM&ab_channel=LeLiberlNews

# How this font seemingly works

The font system is complicated. 
The .fnt file contains a list of characters and their attributes. That structure is 0x18 bytes long.\
![image](https://user-images.githubusercontent.com/69110695/173181150-c4cbe3c0-29fa-4ea1-b933-d82b8e8d2fdc.png)

The first 4 bytes are the unicode of the character (here 0x20, space); \
the following 4 bytes are an integer which we will call Type, which can be 0 1 2 4 in the base font, but could actually be 3, 5, 6, 7 as well (which would be a mix of 1/2/4 together, I didn't think about the implications yet.)\
The two shorts after are the origin X and Y in the GNF file in pixels. \
The two shorts after are the width (0xC) and height (0xE).\
The short after is either 0x100 or 0x200 (maybe 0x300 works for blue too, no idea), 0x100 is green, 0x200 is red, and they're used to separate green and red characters with bit masking.\
The short after (0x12) is used in some kerning calculations although its use is not clear, essentially affecting the spacing in dialogs (or, I think so)\
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

Now the behavior will be different depending on the Type of the character (0,1,2,4). Usually Kanjis are 0, letters from the greek alphabet are 1, 2 and 4 are for letters with a significant space at the left and at the right.

My understanding is that a constant in the exe currently set to 0x30 in the JP eboot governs the distance between the center of one character and the next one. If Type 0 is specified, it is the full distance 0x30 that is used, and if Type is 1, it's half that distance. (By the way this behavior doesn't seem to apply in the context of battle info box...). The byte 0x12 could also in part control the spacing in the dialogs, but I don't think it can reduce it, only increase it (and even that, I'm not so sure).
To have a completely controllable kerning it seems the only solution at the moment is setting the 0x30 constant to 0, which sucks.

This forms the basis of the first component of the spacing, and there is a second one:

![image](https://user-images.githubusercontent.com/69110695/173181645-eac917e3-145e-4a3f-aaa9-191bdaff1844.png)

That logic is as follows: \
first the bitmap width is obtained and divided by 2. (it checks the fact that the distance revolves around the center of the characters)\
If the context is battle info, it will only add the 0x12 byte to the halfwidth.\
In other contexts, for Type 2, the halfwidth will be returned directly\
for Type = 1,\
the constant in exe is retrieved and divided by 2, then the exact same equation as before appears except the result is divided by 2 and added to the halfwidth before being returned in XMM0.\
For Type 4, Same as Type 1 except we remove the bitmap width to it and add that result to half width. 

Type 4 has been observed as leaving a huge space left to the character being change.

Finally, outside of this function, we have this block: \
![image](https://user-images.githubusercontent.com/69110695/173181958-c81ac857-8474-446d-b45f-39b4cba48db3.png)
Which means that the first component of the spacing is multiplied by 1.0 before being added to the second component, then another value is multiplying the total.

This means the spacing, or actually the position of the character, is seemingly controlled by two components, the first and the second "spacing" (however everything is still messy)

Right now I don't have the knowledge to understand why it is that complicated (and basically not usable for variable width font in dialog unless you set some constant to 0).

Note: Another thing that is really bothering: even though you can adjust the y position of the character using the 0x14 byte, for some reason there will be 1 or 2 pixel off (up or down) probably due to some clamping along the way. This is probably what is also happening in the JP base font where the characters are clearly misaligned. This seems like an actual bug for me that should be solved on falcom's end (not that they care though), the workaround we found was to align the character in the GNF/TGA file and set the Y position to 0 for all, which makes us lose a bit of space in the texture technically but there is still plenty (especially if blue is usable)
