# Keybinder

This is a prototype project I developed to address my keyboard remapping needs.

I started using an IBM Model M keyboard, which lacks the LEFTMETA (WIN) key—a feature I find essential for switching between applications.

At the moment, the remapping is applied to the first detected keyboard. However, in the future, I plan to enhance the code to support selecting specific keyboards and associating them with individual profiles.

# Peresquites

### Libevdev

On Fedora install it as follows:

```bash
sudo dnf install libevdev-devel
```

On Debian/Ubuntu install it as follows:

```bash
sudo apt install libevdev-dev
```


### Other

- Ninja
- Cmake