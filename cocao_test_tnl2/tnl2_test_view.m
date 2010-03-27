//
//  tnl2_test_view.m
//  tnl2_test_cocoa
//
//  Created by Mark Frohnmayer on 3/19/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "tnl2_test_view.h"

static unsigned view_index = 0;
extern void *init_game(int server, unsigned index);
extern void shutdown_game(void *the_game);
extern void tick_game(void *the_game, unsigned index);
extern void render_game_scene(void *the_game, unsigned width, unsigned height);
extern void click_game(void *the_game, float x, float y);

@implementation tnl2_test_view

- (id) initWithFrame: (NSRect) frame
{
	GLuint attribs[] = 
	{
		NSOpenGLPFANoRecovery,
		NSOpenGLPFAWindow,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFAAccumSize, 0,
		0
	};
	NSOpenGLPixelFormat* fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: (NSOpenGLPixelFormatAttribute*) attribs]; 
	[super initWithFrame:frame pixelFormat: [fmt autorelease]];
	
	[NSTimer scheduledTimerWithTimeInterval: 0.02 target: self selector:@selector(tick) userInfo:NULL repeats: true];
	
	if (!fmt)
		NSLog(@"No OpenGL pixel format");
	
	_view_index = view_index++;
	_game = init_game(_view_index == 0, _view_index);
	
	return self;
}

- (void) tick
{
	tick_game(_game, _view_index);
	[self setNeedsDisplay: YES];
}

- (void) awakeFromNib
{
	
}

/*
 Override the view's drawRect: to draw our GL content.
 */	 

- (void) drawRect: (NSRect) rect
{
	render_game_scene(_game, (unsigned) rect.size.width, (unsigned) rect.size.height);
	[[self openGLContext] flushBuffer];
}

- (void)mouseDown:(NSEvent *)theEvent
{
	NSPoint mouseLoc;
	mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	NSRect tempRect = [self bounds];
	float x = mouseLoc.x / NSWidth(tempRect);
	float y = mouseLoc.y / NSHeight(tempRect);
	click_game(_game, x, 1 - y);
}

@end
