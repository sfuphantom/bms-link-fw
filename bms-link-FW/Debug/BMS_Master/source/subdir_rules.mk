################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
BMS_Master/source/%.obj: ../BMS_Master/source/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ti-cgt-arm_18.12.4.LTS/bin/armcl" -mv7R4 --code_state=32 --float_support=VFPv3D16 --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW" --include_path="C:/ti/Hercules/F021 Flash API/02.01.01/source" --include_path="C:/ti/Hercules/F021 Flash API/02.01.01/include" --include_path="C:/ti/ti-cgt-arm_18.12.4.LTS/include" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/BMS_Master/include" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/BMS_Master/source" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/Phantom_drivers/include" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/Phantom_drivers/source" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/Test" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/Phantom_Hardware" -g --diag_warning=225 --diag_wrap=off --display_error_number --enum_type=packed --abi=eabi --preproc_with_compile --preproc_dependency="BMS_Master/source/$(basename $(<F)).d_raw" --obj_directory="BMS_Master/source" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

BMS_Master/source/%.obj: ../BMS_Master/source/%.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ti-cgt-arm_18.12.4.LTS/bin/armcl" -mv7R4 --code_state=32 --float_support=VFPv3D16 --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW" --include_path="C:/ti/Hercules/F021 Flash API/02.01.01/source" --include_path="C:/ti/Hercules/F021 Flash API/02.01.01/include" --include_path="C:/ti/ti-cgt-arm_18.12.4.LTS/include" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/BMS_Master/include" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/BMS_Master/source" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/Phantom_drivers/include" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/Phantom_drivers/source" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/Test" --include_path="C:/src/TeamPhantom/bms-link-fw-merge/bms-link-fw/bms-link-FW/Phantom_Hardware" -g --diag_warning=225 --diag_wrap=off --display_error_number --enum_type=packed --abi=eabi --preproc_with_compile --preproc_dependency="BMS_Master/source/$(basename $(<F)).d_raw" --obj_directory="BMS_Master/source" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


