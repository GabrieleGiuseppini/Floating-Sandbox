<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:wix="http://schemas.microsoft.com/wix/2006/wi"
                xmlns="http://schemas.microsoft.com/wix/2006/wi"
                exclude-result-prefixes="xsl wix">

    <xsl:variable name="ProductRegistryKey" select="'Software\Gabriele Giuseppini\Floating Sandbox V1'" />

    <xsl:output method="xml" indent="yes" omit-xml-declaration="yes" />

    <xsl:strip-space elements="*" />

    <xsl:template match="@*|node()">
        <xsl:copy>
            <xsl:apply-templates select="@*|node()" />
        </xsl:copy>
    </xsl:template>

    <xsl:template match="wix:Component/wix:File[@Id]">
        <xsl:copy>
            <xsl:apply-templates select="@*" />
            <xsl:attribute name="KeyPath">
                <xsl:text>no</xsl:text>
            </xsl:attribute>
        </xsl:copy>
        <xsl:element name="RegistryValue">
            <xsl:attribute name="Root">
                <xsl:text>HKCU</xsl:text>
            </xsl:attribute>
            <xsl:attribute name="Key">
                <xsl:value-of select="$ProductRegistryKey" />
            </xsl:attribute>
            <xsl:attribute name="Name">
                <xsl:text>Installed</xsl:text>
            </xsl:attribute>
            <xsl:attribute name="Type">
                <xsl:text>integer</xsl:text>
            </xsl:attribute>
            <xsl:attribute name="Value">
                <xsl:text>1</xsl:text>
            </xsl:attribute>
            <xsl:attribute name="KeyPath">
                <xsl:text>yes</xsl:text>
            </xsl:attribute>
        </xsl:element>
    </xsl:template>

    <xsl:template match="wix:Component/wix:File[not(@Id)]">
        <!-- Placeholder for handling anything that should happen
         different for wix:File without @Id attribute.  -->
        <xsl:copy>
            <xsl:apply-templates select="@*" />
        </xsl:copy>
    </xsl:template>

    <!--Give Component ID prefix C_-->
    <!--
    <xsl:template match="wix:Component/@Id">
        <xsl:attribute name="{name()}">
            <xsl:value-of select="concat('Comp_', .)" />
        </xsl:attribute>
    </xsl:template>
    -->

    <!--Give Componentref ID prefix C_-->
    <!--
    <xsl:template match="wix:ComponentRef/@Id">
        <xsl:attribute name="{name()}">
            <xsl:value-of select="concat('C_', .)" />
        </xsl:attribute>
    </xsl:template>
    -->

    <!--Give Directory ID prefix Dir_-->
    <xsl:template match="wix:Directory/@Id">
        <xsl:attribute name="{name()}">
            <xsl:value-of select="concat('Dir_', .)" />
        </xsl:attribute>
    </xsl:template>

    <!--Give File ID prefix File_-->
    <xsl:template match="wix:File/@Id">
        <xsl:attribute name="{name()}">
            <xsl:value-of select="concat('File_', .)" />
        </xsl:attribute>
    </xsl:template>

    <!-- Directory match -->
    <!--

    <xsl:template match="wix:Directory">
        <xsl:copy>
            <xsl:apply-templates select="@*" />

            <xsl:element name="Component">
                <xsl:attribute name="Id">
                    <xsl:value-of select="concat('Comp_', @Id)" />
                </xsl:attribute>
                <xsl:attribute name="Guid">
                    <xsl:text>*</xsl:text>
                </xsl:attribute>

                <xsl:element name="RemoveFolder">
                    <xsl:attribute name="Id">
                        <xsl:value-of select="@Id" />
                    </xsl:attribute>
                    <xsl:attribute name="Directory">
                        <xsl:value-of select="concat('Dir_', @Id)" />
                    </xsl:attribute>
                    <xsl:attribute name="On">
                        <xsl:text>uninstall</xsl:text>
                    </xsl:attribute>
                </xsl:element>
                <RegistryValue Root="HKCU" Key="Software\Gabriele Giuseppini\Floating Sandbox" Name="Installed" Type="integer" Value="1" KeyPath="yes" />
            </xsl:element>
            <xsl:apply-templates select="node()" />
        </xsl:copy>
    </xsl:template>
    -->

    <!-- Component to remove all child directories -->

    <xsl:template match="wix:DirectoryRef">

        <xsl:copy>
            <xsl:apply-templates select="@*|node()" />

            <xsl:element name="Component">
                <xsl:attribute name="Id">
                    <xsl:value-of select="concat(@Id, '_children_removal_comp')" />
                </xsl:attribute>
                <xsl:attribute name="Guid">
                    <xsl:value-of select="concat('$(var.', @Id, 'ChildRemovalCompGuid)')" />
                </xsl:attribute>

                <xsl:apply-templates select=".//wix:Directory" mode="DirectoryRemoval" />

                <xsl:element name="RegistryValue">
                    <xsl:attribute name="Root">
                        <xsl:text>HKCU</xsl:text>
                    </xsl:attribute>
                    <xsl:attribute name="Key">
                        <xsl:value-of select="$ProductRegistryKey" />
                    </xsl:attribute>
                    <xsl:attribute name="Name">
                        <xsl:text>Installed</xsl:text>
                    </xsl:attribute>
                    <xsl:attribute name="Type">
                        <xsl:text>integer</xsl:text>
                    </xsl:attribute>
                    <xsl:attribute name="Value">
                        <xsl:text>1</xsl:text>
                    </xsl:attribute>
                    <xsl:attribute name="KeyPath">
                        <xsl:text>yes</xsl:text>
                    </xsl:attribute>
                </xsl:element>

            </xsl:element>

        </xsl:copy>


    </xsl:template>

    <xsl:template match="wix:Directory" mode="DirectoryRemoval">
        <xsl:element name="RemoveFolder">
            <xsl:attribute name="Id">
                <xsl:value-of select="@Id" />
            </xsl:attribute>
            <xsl:attribute name="Directory">
                <xsl:value-of select="concat('Dir_', @Id)" />
            </xsl:attribute>
            <xsl:attribute name="On">
                <xsl:text>uninstall</xsl:text>
            </xsl:attribute>
        </xsl:element>

    </xsl:template>


    <!-- TODOTEST

    <xsl:template match="wix:ComponentGroup">
        <ComponentGroup Id="{@Id}">
            <xsl:apply-templates select="//wix:Component" mode="ComponentGroup" />
            <xsl:apply-templates select="//wix:Directory" mode="ComponentGroup" />
        </ComponentGroup>
    </xsl:template>

    <xsl:template match="wix:Component" mode="ComponentGroup">
        <xsl:element name="ComponentRef">
            <xsl:attribute name="Id">
                <xsl:value-of select="concat('Comp_', @Id)" />
            </xsl:attribute>
        </xsl:element>
    </xsl:template>

    <xsl:template match="wix:Directory" mode="ComponentGroup">
        <xsl:element name="ComponentRef">
            <xsl:attribute name="Id">
                <xsl:value-of select="concat('Comp_', @Id)" />
            </xsl:attribute>
        </xsl:element>
    </xsl:template>
    -->

</xsl:stylesheet>