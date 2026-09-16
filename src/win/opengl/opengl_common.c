
void win_begin_frame(window* win) {
    glClearColor(
        win->clear_color.x, win->clear_color.y,
        win->clear_color.z, win->clear_color.w
    );
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, (GLsizei)win->width, (GLsizei)win->height);
}

