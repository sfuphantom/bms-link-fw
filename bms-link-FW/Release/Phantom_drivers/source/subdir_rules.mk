################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Phantom_drivers/source/%.obj: ../Phantom_drivers/source/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ti-cgt-arm_18.12.4.LTS/bin/armcl" -mv7R4 --code_state=32 --float_support=VFPv3D16 -O2 --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW" --include_path="C:/ti/ti-cgt-arm_18.12.4.LTS/include" --diag_warning=225 --diag_wrap=off --display_error_number --enum_type=packed --abi=eabi --preproc_with_compile --preproc_dependency="Phantom_drivers/source/$(basename $(<F)).d_raw" --obj_directory="Phantom_drivers/source" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


