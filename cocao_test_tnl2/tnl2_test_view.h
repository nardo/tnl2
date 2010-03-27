//
//  tnl2_test_view.h
//  tnl2_test_cocoa
//
//  Created by Mark Frohnmayer on 3/19/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface tnl2_test_view : NSOpenGLView {
	unsigned _view_index;
	void *_game;
}
- (void) tick;
- (void)mouseDown:(NSEvent *)theEvent;

@end
