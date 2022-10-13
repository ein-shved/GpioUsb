let pkgs = import <nixpkgs> {};
  stdenv = pkgs.stdenv;
  stflash = "${pkgs.stlink}/bin/st-flash";
  flasher = pkgs.writeShellScriptBin "flasher" ''
    out="$1"
    nfiles="$(echo $out/*.bin | wc -w)"
    if [ x"$nfiles" == x"1" ]; then
      ${stflash} --reset write $out/*.bin 0x8000000 || true
    fi
  '';
in
stdenv.mkDerivation {
  name = "stm-gpiousb-0.1";
  buildInputs = with pkgs; [
    pkgs.libusb1
    gcc-arm-embedded
  ];
  src = ./.;
  installPhase = ''
    mkdir -p "$out"
    cp ./build/*.bin "$out"
    cp ./build/*.elf "$out"
    echo '#!/bin/bash' > $out/flasher
    echo "exec ${flasher}/bin/flasher \"$out\"" >> "$out/flasher"
    chmod +x "$out/flasher"
  '';
  postInstall = "true";
}
