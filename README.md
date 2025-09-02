# Micropython-For-ps4
to access the repl use netcat:

`nc <ps4-ip> 9025`

## Known Limitations

- Limited to 64KB heap (can be increased in source)
- No filesystem access yet
- Single client connection at a time

## Future Enhancements

- [ ] Increase heap size for larger programs
- [ ] Add PS4-specific Python modules
- [ ] USB keyboard support for local REPL
- [ ] File system access
- [ ] Multiple simultaneous connections
- [ ] Graphics and controller APIs
