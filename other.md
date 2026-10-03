```cpp
int number = SDL_GetNumRenderDrivers();
printf("%d", number);
for (int i = 0; i < number; i++) {
  const char *name = SDL_GetRenderDriver(i);
  printf("%s ", name);
}
```

```sh
cmake --preset clang - ninja
 cmake --build build
```