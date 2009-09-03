/*
Copyright (C) 2009 Parallel Realities

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "../headers.h"

#include "../audio/audio.h"
#include "../graphics/animation.h"
#include "../system/properties.h"
#include "../entity.h"
#include "../event/trigger.h"
#include "../hud.h"

extern Entity *self;

static void wait(void);
static void init(void);
static void touch(Entity *);

Entity *addArrowTarget(int x, int y, char *name)
{
	Entity *e = getFreeEntity();

	if (e == NULL)
	{
		printf("No free slots to add an Arrow Target\n");

		exit(1);
	}

	loadProperties(name, e);

	e->x = x;
	e->y = y;

	e->action = &init;

	e->draw = &drawLoopingAnimationToMap;
	e->touch = &touch;

	e->type = KEY_ITEM;

	setEntityAnimation(e, STAND);

	return e;
}

static void wait()
{
	if (self->thinkTime > 0)
	{
		self->thinkTime--;

		if (self->thinkTime <= 0)
		{
			self->thinkTime = 0;

			self->active = self->active == TRUE ? FALSE : TRUE;

			setEntityAnimation(self, self->active == FALSE ? STAND : WALK);

			activateEntitiesWithRequiredName(self->objectiveName, self->active);

			playSoundToMap("sound/common/switch.ogg", -1, self->x, self->y, 0);

			stopSound(self->startX);
		}
	}
}

static void init()
{
	setEntityAnimation(self, self->active == FALSE ? STAND : WALK);

	self->action = &wait;
}

static void touch(Entity *other)
{
	int remaining;
	Entity *temp;

	if (strcmpignorecase(other->name, self->requires) == 0)
	{
		if (other->type == PROJECTILE)
		{
			temp = self;

			self = other;

			self->die();

			self = temp;
		}

		/* Don't toggle if a timer */

		if (self->maxThinkTime != 0 && self->thinkTime != 0)
		{
			self->thinkTime = self->maxThinkTime;

			return;
		}

		remaining = countSiblings(self);

		if (remaining == 0)
		{
			self->active = self->active == TRUE ? FALSE : TRUE;

			setEntityAnimation(self, self->active == FALSE ? STAND : WALK);

			activateEntitiesWithRequiredName(self->objectiveName, self->active);

			self->thinkTime = self->maxThinkTime;

			if (self->maxThinkTime != 0)
			{
				self->startX = playSoundToMap("sound/common/tick.ogg", -1, self->x, self->y, -1);
			}
		}

		else
		{
			setInfoBoxMessage(30,  _("%d more to go..."), remaining);
		}

		playSoundToMap("sound/common/switch.ogg", -1, self->x, self->y, 0);
	}
}