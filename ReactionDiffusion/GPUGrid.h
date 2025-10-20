//
///************************************************************
//* GrayScott
//*
//*
//************************************************************/
//#pragma once
//#include "GridCS.h"
//#include "shaders\TextureShader.h"
//#include "GrayScottReactionDiffusion.h"
//
//class GrayScottGPUGrid : public GrayScottReactionDiffusion
//{
//public:
//	GrayScottGPUGrid(SimulationDomain* domain, std::map<string, CellDataType> params, int width, int height)
//		: GrayScottReactionDiffusion(domain, params), width(width), height(height)
//	{
//		updateParams();
//		initSim();
//		isGPUEnabled = true;
//	}
//
//	~GrayScottGPUGrid()
//	{}
//
//	void doSimulate() override
//	{
//		cs->bind(width, height);
//		cs->dispatch(width, height, 1);
//		cs->wait();
//	}
//
//	void initSim() override
//	{
//		GrayScottReactionDiffusion::initSim();
//
//		cs = std::make_unique<GrayScottGridCS>(this);
//		cs->createTextureArray(width, height);
//
//		std::unique_ptr<TextureShader> shader = std::make_unique<TextureShader>();
//		shader->texture = cs->texture;
//		domain->shader = std::move(shader);
//	}
//
//	void updateColors() override
//	{
//		domain->shader->setColorMapTexture(domain->colorMap.getTextureID());
//	}
//
//	const int width, height;
//	std::unique_ptr<GrayScottGridCS> cs;
//};
//
///************************************************************
//* DecayDiffusion
//*
//*
//************************************************************/
//#include "DecayDiffusion.h"
//
//class DecayDiffusionGPUGrid : public DecayDiffusion
//{
//public:
//	DecayDiffusionGPUGrid(SimulationDomain* domain, std::map<string, CellDataType> params, int width, int height)
//		: DecayDiffusion(domain, params), width(width), height(height)
//	{
//		updateParams();
//		initSim();
//		isGPUEnabled = true;
//	}
//
//	~DecayDiffusionGPUGrid()
//	{}
//
//	void doSimulate() override
//	{
//		cs->bind(width, height);
//		cs->dispatch(width, height, 1);
//		cs->wait();
//	}
//
//	void initSim() override
//	{
//		DecayDiffusion::initSim();
//
//		cs = std::make_unique<DecayDiffusionGridCS>(this);
//		cs->createTextureArray(width, height);
//
//		std::unique_ptr<TextureShader> shader = std::make_unique<TextureShader>();
//		shader->texture = cs->texture;
//		domain->shader = std::move(shader);
//	}
//
//	void updateColors() override
//	{
//		domain->shader->setColorMapTexture(domain->colorMap.getTextureID());
//	}
//
//	const int width, height;
//	std::unique_ptr<DecayDiffusionGridCS> cs;
//};
//
///************************************************************
//* Kondo
//*
//*
//************************************************************/
//#include "KondoReactionDiffusion.h"
//
//class KondoGPUGrid : public KondoReactionDiffusion
//{
//public:
//	KondoGPUGrid(SimulationDomain* domain, std::map<string, CellDataType> params, int width, int height)
//		: KondoReactionDiffusion(domain, params), width(width), height(height)
//	{
//		updateParams();
//		initSim();
//		isGPUEnabled = true;
//	}
//
//	~KondoGPUGrid()
//	{}
//
//	void doSimulate() override
//	{
//		cs->bind(width, height);
//		cs->dispatch(width, height, 1);
//		cs->wait();
//	}
//
//	void initSim() override
//	{
//		KondoReactionDiffusion::initSim();
//
//		cs = std::make_unique<KondoGridCS>(this);
//		cs->createTextureArray(width, height);
//
//		std::unique_ptr<TextureShader> shader = std::make_unique<TextureShader>();
//		shader->texture = cs->texture;
//		domain->shader = std::move(shader);
//	}
//
//	void updateColors() override
//	{
//		domain->shader->setColorMapTexture(domain->colorMap.getTextureID());
//	}
//
//	const int width, height;
//	std::unique_ptr<KondoGridCS> cs;
//};
//
///************************************************************
//* Turing
//*
//*
//************************************************************/
//#include "TuringReactionDiffusion.h"
//
//class TuringGPUGrid : public TuringReactionDiffusion
//{
//public:
//	TuringGPUGrid(SimulationDomain* domain, std::map<string, CellDataType> params, int width, int height)
//		: TuringReactionDiffusion(domain, params), width(width), height(height)
//	{
//		updateParams();
//		initSim();
//		isGPUEnabled = true;
//	}
//
//	~TuringGPUGrid()
//	{}
//
//	void doSimulate() override
//	{
//		cs->bind(width, height);
//		cs->dispatch(width, height, 1);
//		cs->wait();
//	}
//
//	void initSim() override
//	{
//		TuringReactionDiffusion::initSim();
//
//		cs = std::make_unique<TuringGridCS>(this);
//		cs->createTextureArray(width, height);
//
//		std::unique_ptr<TextureShader> shader = std::make_unique<TextureShader>();
//		shader->texture = cs->texture;
//		domain->shader = std::move(shader);
//	}
//
//	void updateColors() override 
//	{
//		domain->shader->setColorMapTexture(domain->colorMap.getTextureID());
//	}
//
//	const int width, height;
//	std::unique_ptr<TuringGridCS> cs;
//};
//
///************************************************************
//* GiererMeinhardt
//*
//*
//************************************************************/
//#include "GiererMeinhardtReactionDiffusion.h"
//
//class GiererMeinhardtGPUGrid : public GiererMeinhardtReactionDiffusion
//{
//public:
//	GiererMeinhardtGPUGrid(SimulationDomain* domain, std::map<string, CellDataType> params, int width, int height)
//		: GiererMeinhardtReactionDiffusion(domain, params), width(width), height(height)
//	{
//		updateParams();
//		initSim();
//		isGPUEnabled = true;
//	}
//
//	~GiererMeinhardtGPUGrid()
//	{}
//
//	void doSimulate() override
//	{
//		cs->bind(width, height);
//		cs->dispatch(width, height, 1);
//		cs->wait();
//	}
//
//	void initSim() override
//	{
//		GiererMeinhardtReactionDiffusion::initSim();
//
//		cs = std::make_unique<GiererMeinhardtCS>(this);
//		cs->createTextureArray(width, height);
//
//		std::unique_ptr<TextureShader> shader = std::make_unique<TextureShader>();
//		shader->texture = cs->texture;
//		domain->shader = std::move(shader);
//	}
//
//	void updateColors() override
//	{
//		domain->shader->setColorMapTexture(domain->colorMap.getTextureID());
//	}
//
//	const int width, height;
//	std::unique_ptr<GiererMeinhardtCS> cs;
//};
//
///************************************************************
//* OsterMurray
//*
//*
//************************************************************/
//#include "OsterMurrayReactionDiffusion.h"
//
//class OsterMurrayGPUGrid : public OsterMurrayReactionDiffusion
//{
//public:
//	OsterMurrayGPUGrid(SimulationDomain* domain, std::map<string, CellDataType> params, int width, int height)
//		: OsterMurrayReactionDiffusion(domain, params), width(width), height(height)
//	{
//		updateParams();
//		initSim();
//		isGPUEnabled = true;
//	}
//
//	~OsterMurrayGPUGrid()
//	{}
//
//	void doSimulate() override
//	{
//		cs->bind(width, height);
//		cs->dispatch(width, height, 1);
//		cs->wait();
//	}
//
//	void initSim() override
//	{
//		OsterMurrayReactionDiffusion::initSim();
//
//		cs = std::make_unique<OsterMurrayCS>(this);
//		cs->createTextureArray(width, height);
//
//		std::unique_ptr<TextureShader> shader = std::make_unique<TextureShader>();
//		shader->texture = cs->texture;
//		domain->shader = std::move(shader);
//	}
//
//	void updateColors() override
//	{
//		domain->shader->setColorMapTexture(domain->colorMap.getTextureID());
//	}
//
//	const int width, height;
//	std::unique_ptr<OsterMurrayCS> cs;
//};