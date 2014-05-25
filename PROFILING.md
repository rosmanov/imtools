# Profiling

Following steps will generate profile data by means of the `gprof` and `gcov` tools:

## 1. Build the project with special configuration

```
./profile-build.sh
```

## 2. Run each binary

Each binary in the `bin` directory must be run before using `gprof` or `gcov` tools. E.g.:

```
bin/immerge  -s -p --old-image=1.jpg --new-image=2.jpg 1_a.jpg images/out/1_a.jpg 1_c.png images/out/1_b.png 1_e.png images/out/1_e.png
```

## 3. Generate the reports

```
./profile-gen.sh
```
