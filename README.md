# Le Frata

![](https://github.com/alexandreboutrik/lefrata-tb/blob/main/media/GIF.gif)

Fork do [Le Frata](https://github.com/LeFrata/lefrata-cbp) utilizando a [termbox](https://github.com/sanko/termbox2.c) ao invés da ncurses.

## Compilação e Execução

Compile o termbox caso não possua:

```
cd termbox  
mkdir build ; cd build  
cmake ..  
make  
cp lib* /usr/local/lib  
cp ../src/termbox.h /usr/local/include
```

Este repositório utiliza o [nobuild](https://github.com/tsoding/nobuild) para compilação:

```
gcc nobuild.c -o nobuild  
./nobuild  
```

Para executar:

```
./bin/frata # ou ./nobuild r
```

## Licença

Este projeto está licenciado sob a [MIT License](https://opensource.org/licenses/MIT). Sinta-se à vontade para usar, modificar e distribuir o código conforme necessário.

## Contato e Contribuição

Se tiver alguma dúvida, sugestão, feedback, ou se deseja contribuir para este projeto, sinta-se à vontade para enviar pull requests ou ir até a página de discussão deste repositório.

