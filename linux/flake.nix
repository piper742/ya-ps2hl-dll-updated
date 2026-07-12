{
  outputs = {nixpkgs, ...}: let
    system = "x86_64-linux";
    np = import nixpkgs {
      localSystem = "x86_64-linux";
    };
    pkgs = np.pkgsi686Linux;
  in {
    devShells.${system}.default = pkgs.mkShell {
      packages = with pkgs; [
        gcc
        mesa-gl-headers
        libGL
        libGLU
        gdb
      ];
    };
  };
}

