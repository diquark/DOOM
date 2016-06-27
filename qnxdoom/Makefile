################################################################
#
# $Id:$
#
# $Log:$
#
CC=  cc

CFLAGS=-5r -fp5 -fpi87 -Ortx -j -Wc,-ei -DRAW_WIDGET
LDFLAGS=-N1m
LIBS=-lphoton_s -lphexlib -lAplib

# subdirectory for objects
OBJ_DIR=obj/release

# subdirectory for binary
BIN_DIR=bin

# not too sophisticated dependency
OBJS=				\
		$(OBJ_DIR)/doomdef.o		\
		$(OBJ_DIR)/doomstat.o		\
		$(OBJ_DIR)/dstrings.o		\
		$(OBJ_DIR)/i_main.o		\
		$(OBJ_DIR)/i_system.o		\
		$(OBJ_DIR)/i_sound.o		\
		$(OBJ_DIR)/i_video.o		\
		$(OBJ_DIR)/i_net.o			\
		$(OBJ_DIR)/tables.o			\
		$(OBJ_DIR)/f_finale.o		\
		$(OBJ_DIR)/f_wipe.o 		\
		$(OBJ_DIR)/d_main.o			\
		$(OBJ_DIR)/d_net.o			\
		$(OBJ_DIR)/d_items.o		\
		$(OBJ_DIR)/g_game.o			\
		$(OBJ_DIR)/m_menu.o			\
		$(OBJ_DIR)/m_misc.o			\
		$(OBJ_DIR)/m_argv.o  		\
		$(OBJ_DIR)/m_bbox.o			\
		$(OBJ_DIR)/m_fixed.o		\
		$(OBJ_DIR)/m_swap.o			\
		$(OBJ_DIR)/m_cheat.o		\
		$(OBJ_DIR)/m_random.o		\
		$(OBJ_DIR)/am_map.o			\
		$(OBJ_DIR)/p_ceilng.o		\
		$(OBJ_DIR)/p_doors.o		\
		$(OBJ_DIR)/p_enemy.o		\
		$(OBJ_DIR)/p_floor.o		\
		$(OBJ_DIR)/p_inter.o		\
		$(OBJ_DIR)/p_lights.o		\
		$(OBJ_DIR)/p_map.o			\
		$(OBJ_DIR)/p_maputl.o		\
		$(OBJ_DIR)/p_plats.o		\
		$(OBJ_DIR)/p_pspr.o			\
		$(OBJ_DIR)/p_setup.o		\
		$(OBJ_DIR)/p_sight.o		\
		$(OBJ_DIR)/p_spec.o			\
		$(OBJ_DIR)/p_switch.o		\
		$(OBJ_DIR)/p_mobj.o			\
		$(OBJ_DIR)/p_telept.o		\
		$(OBJ_DIR)/p_tick.o			\
		$(OBJ_DIR)/p_saveg.o		\
		$(OBJ_DIR)/p_user.o			\
		$(OBJ_DIR)/r_bsp.o			\
		$(OBJ_DIR)/r_data.o			\
		$(OBJ_DIR)/r_draw.o			\
		$(OBJ_DIR)/r_main.o			\
		$(OBJ_DIR)/r_plane.o		\
		$(OBJ_DIR)/r_segs.o			\
		$(OBJ_DIR)/r_sky.o			\
		$(OBJ_DIR)/r_things.o		\
		$(OBJ_DIR)/w_wad.o			\
		$(OBJ_DIR)/wi_stuff.o		\
		$(OBJ_DIR)/v_video.o		\
		$(OBJ_DIR)/st_lib.o			\
		$(OBJ_DIR)/st_stuff.o		\
		$(OBJ_DIR)/hu_stuff.o		\
		$(OBJ_DIR)/hu_lib.o			\
		$(OBJ_DIR)/s_sound.o		\
		$(OBJ_DIR)/z_zone.o			\
		$(OBJ_DIR)/info.o				\
		$(OBJ_DIR)/sounds.o

all: $(BIN_DIR) $(OBJ_DIR) $(BIN_DIR)/phdoom

install: $(BIN_DIR)/phdoom
	cp $< /usr/bin/photon
	mkdir -p /usr/local/share/doom
	mkdir -p $(HOME)/.photon/phdoom
	cp ./config/.doomrc $(HOME)/.photon/phdoom

clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/phdoom

$(BIN_DIR)/phdoom: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
	usemsg $@ use.msg
	bindres $@ Icon.wgti

$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

#############################################################
#
#############################################################