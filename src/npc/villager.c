#include "../headers.h"

#include "../graphics/animation.h"
#include "../system/properties.h"
#include "../entity.h"
#include "../system/random.h"
#include "../collisions.h"
#include "../audio/audio.h"
#include "../event/script.h"
#include "../hud.h"

extern Entity *self;
extern Game game;

static void wait(void);
static void talk(int);
static void touch(Entity *);

Entity *addVillager(int x, int y, char *name)
{
	Entity *e = getFreeEntity();

	if (e == NULL)
	{
		printf("No free slots to add a Villager\n");

		exit(1);
	}

	loadProperties(name, e);

	e->x = x;
	e->y = y;

	e->action = &wait;

	e->draw = &drawLoopingAnimationToMap;
	e->activate = &talk;
	e->touch = &touch;

	e->type = ENEMY;

	setEntityAnimation(e, STAND);

	return e;
}

static void wait()
{
	checkToMap(self);
}

static void talk(int val)
{
	loadScript(self->requires);

	readNextScriptLine();
}

static void touch(Entity *other)
{
	if (other->type == PLAYER && game.showHints == TRUE)
	{
		setInfoBoxMessage(0,  _("Press Action to talk to %s"), self->objectiveName);
	}
}